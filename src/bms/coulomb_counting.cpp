#include "bms/coulomb_counting.h"
#include "utils/soc_lookup.h"

#include <algorithm>

namespace param
{
    float b_as = 0.0f;
    float C_as = BMS_INITIAL_CAPACITY_AS * 0.5f; //Better have initial capacity too low
    float soh = 1.0f;
    bool have_low_anchor = false;
    float q_low_as = 0.0f;
    float soc_low_anchor = 0.0f;
    bool was_above_high_set = false;
    float q_as = 0.0f;
    bool ocv_valid = false;
    float soc_ocv = 0.0f;
    float soc_cc = 0.0f;
    CoulombCountingState coulomb_state = CoulombCountingState::INIT;
}

namespace
{
constexpr float soc_low_set = 0.20f;
constexpr float soc_high_set = 0.80f;
constexpr float soc_hyst = 0.05f;
constexpr float soc_low_reset = soc_low_set + soc_hyst;
constexpr float soc_high_reset = soc_high_set - soc_hyst;

constexpr float gamma_b = 0.05f;
constexpr float alpha_C = 0.10f;

constexpr float C_rated_as = CC_CAP_RATED_AS;
constexpr float C_min = 0.50f * C_rated_as;
constexpr float C_max = 1.20f * C_rated_as;

constexpr float b_frac = 1.50f;
constexpr float b_step_max_frac = 0.01f;

float clamp(float x, float lo, float hi)
{
    if (x < lo)
    {
        return lo;
    }
    if (x > hi)
    {
        return hi;
    }
    return x;
}
} // namespace

void CoulombCounting::initialise(float b_as,
                                 float C_as,
                                 float soh,
                                 bool have_low_anchor,
                                 float q_low_as,
                                 float soc_low_anchor,
                                 bool was_above_high_set)
{
    b_as_ = b_as;
    C_as_ = C_as;
    soh_ = soh;
    have_low_anchor_ = have_low_anchor;
    q_low_as_ = q_low_as;
    soc_low_anchor_ = soc_low_anchor;
    was_above_high_set_ = was_above_high_set;
    ocv_valid_ = false;
    soc_ocv_ = 0.0f;
    soc_cc_ = 0.0f;
    q_as_ = param::as;
    ocv_rest_timer_ = 0.0f;

    if (C_as_ > 0.0f)
    {
        soc_cc_ = (q_as_ - b_as_) / C_as_;
        soc_cc_ = std::clamp(soc_cc_, 0.0f, 1.0f);
    }
    else
    {
        soc_cc_ = 0.0f;
    }

    last_update_ms_ = 0U;
    if (C_as_ > 0.0f)
    {
        state_ = CoulombCountingState::OPERATING;
    }
    else
    {
        state_ = CoulombCountingState::INIT;
    }
    persistent_loaded_ = true;
    publish_params_();
}

void CoulombCounting::update(float v_min, float v_max, float avg_temp_c)
{
    if (param::state == ShuntState::FAULT)
    {
        state_ = CoulombCountingState::FAULT;
        publish_params_();
        return;
    }

    if (state_ == CoulombCountingState::INIT)
    {
        if (persistent_loaded_ && C_as_ > 0.0f)
        {
            state_ = CoulombCountingState::OPERATING;
        }
        else
        {
            publish_params_();
            return;
        }
    }

    const uint32_t now = millis();
    if (last_update_ms_ == 0U)
    {
        last_update_ms_ = now;
        publish_params_();
        return;
    }

    const uint32_t delta_ms = now - last_update_ms_;
    last_update_ms_ = now;
    if (delta_ms == 0U)
    {
        publish_params_();
        return;
    }

    const float dt_s = static_cast<float>(delta_ms) / 1000.0f;

    // Coulomb integration (charge positive, discharge negative).
    q_as_ = param::as;
    if (C_as_ > 0.0f)
    {
        soc_cc_ = (q_as_ - b_as_) / C_as_;
        soc_cc_ = std::clamp(soc_cc_, 0.0f, 1.0f);
    }
    else
    {
        soc_cc_ = 0.0f;
    }

    // OCV validity based on rest time.
    if (std::fabs(param::current) < CC_REST_THRESHOLD_A)
    {
        ocv_rest_timer_ += dt_s;
    }
    else
    {
        ocv_rest_timer_ = 0.0f;
    }

    ocv_valid_ = (ocv_rest_timer_ > CC_REST_TIME_MIN_S);
    if (ocv_valid_)
    {
        const float soc_hi = SOC_FROM_OCV_TEMP(avg_temp_c, v_max) / 100.0f;
        const float soc_lo = SOC_FROM_OCV_TEMP(avg_temp_c, v_min) / 100.0f;
        if (soc_hi >= soc_high_set)
        {
            soc_ocv_ = soc_hi;
        }
        else if (soc_lo <= soc_low_set)
        {
            soc_ocv_ = soc_lo;
        }
        else
        {
            soc_ocv_ = 0.5f * (soc_hi + soc_lo);
        }
        soc_ocv_ = std::clamp(soc_ocv_, 0.0f, 1.0f);
    }
    else
    {
        soc_ocv_ = 0.0f;
    }

    // ============================================================
    // RESET LOW ANCHOR when coming from high_set and going below high_reset
    // ============================================================
    if (was_above_high_set_ && (soc_cc_ <= soc_high_reset))
    {
        have_low_anchor_ = false;
        was_above_high_set_ = false;
    }

    // ============================================================
    // Track that we reached high region at least once (persistent)
    // ============================================================
    if (soc_cc_ >= soc_high_set)
    {
        was_above_high_set_ = true;
    }

    // ============================================================
    // 1) LOW REGION: anchor + b update (with clamping)
    // ============================================================
    if (ocv_valid_ && (soc_cc_ <= soc_low_set))
    {
        // Residual: e = q - (b + C*SOC)
        const float e = q_as_ - (b_as_ + C_as_ * soc_ocv_);

        // Raw update
        float delta_b = gamma_b * e;

        // Step clamp on b update
        const float b_step_max = b_step_max_frac * C_as_;
        delta_b = clamp(delta_b, -b_step_max, b_step_max);

        // Apply update
        b_as_ = b_as_ + delta_b;

        // Value clamp for b (relative to C)
        const float b_min = -b_frac * C_as_;
        const float b_max = b_frac * C_as_;
        b_as_ = clamp(b_as_, b_min, b_max);

        have_low_anchor_ = true;
        q_low_as_ = q_as_;
        soc_low_anchor_ = soc_cc_;
    }

    // ============================================================
    // 2) HIGH REGION: capacity update (with clamping) + SOH update
    // ============================================================
    if (ocv_valid_ && have_low_anchor_ && (soc_cc_ >= soc_high_set))
    {
        // Two-point slope
        float C_new = (q_as_ - q_low_as_) / (soc_ocv_ - soc_low_anchor_);

        // Reject gross outliers (optional but recommended)
        C_new = clamp(C_new, 0.30f * C_rated_as, 1.50f * C_rated_as);

        // EMA update
        C_as_ = (1.0f - alpha_C) * C_as_ + alpha_C * C_new;

        // Hard clamp
        C_as_ = clamp(C_as_, C_min, C_max);

        // SOH update (0..1, can exceed 1 if you allow it; usually clamp to 1.0)
        if (C_rated_as > 0.0f)
        {
            soh_ = C_as_ / C_rated_as;
        }
        else
        {
            soh_ = 0.0f;
        }
        soh_ = clamp(soh_, 0.0f, 1.5f);
    }

    publish_params_();
}

void CoulombCounting::publish_params_()
{
    param::b_as = b_as_;
    param::C_as = C_as_;
    param::soh = soh_;
    param::have_low_anchor = have_low_anchor_;
    param::q_low_as = q_low_as_;
    param::soc_low_anchor = soc_low_anchor_;
    param::was_above_high_set = was_above_high_set_;
    param::q_as = q_as_;
    param::ocv_valid = ocv_valid_;
    param::soc_ocv = soc_ocv_;
    param::soc_cc = soc_cc_;
    param::coulomb_state = state_;
}
