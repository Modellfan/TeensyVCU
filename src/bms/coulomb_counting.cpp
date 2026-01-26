#include "bms/coulomb_counting.h"
#include "utils/soc_lookup.h"

#include <algorithm>

namespace param
{
    float cap_est_as = BMS_INITIAL_CAPACITY_AS;
    float q_offset_as = 0.0f;
    float q_total_init = 0.0f;
    bool recal_active = false;
    float recal_start_q = 0.0f;
    float soh = 1.0f;
    float as_session = 0.0f;
    float ocv_rest_timer = 0.0f;
    float soc_cc = 0.0f;
    float soc_est = 0.0f;
    float q_total = 0.0f;
    CoulombCountingState coulomb_state = CoulombCountingState::INIT;
}

void CoulombCounting::initialise(float cap_est_as,
                                 float q_total_init,
                                 bool recal_active,
                                 float recal_start_q)
{
    cap_est_as_ = cap_est_as;
    q_total_init_ = q_total_init;
    recal_active_ = recal_active;
    recal_start_q_ = recal_start_q;
    q_offset_as_ = 0.0f;
    as_session_ = 0.0f;
    ocv_rest_timer_ = 0.0f;
    q_total_ = q_total_init_;
    if (cap_est_as_ > 0.0f)
    {
        soc_cc_ = q_total_ / cap_est_as_;
        soc_cc_ = std::clamp(soc_cc_, 0.0f, 1.0f);
        soc_est_ = (q_total_ + q_offset_as_) / cap_est_as_;
        soc_est_ = std::clamp(soc_est_, 0.0f, 1.0f);
        soc_est_old_ = soc_est_;
    }
    else
    {
        soc_cc_ = 0.0f;
        soc_est_ = 0.0f;
        soc_est_old_ = 0.0f;
    }

    last_update_ms_ = 0U;
    soh_ = (CC_CAP_RATED_AS > 0.0f) ? (cap_est_as_ / CC_CAP_RATED_AS) : 0.0f;
    if (cap_est_as_ > 0.0f)
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
        if (persistent_loaded_ && cap_est_as_ > 0.0f)
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

    // soc_est_ is the corrected soc during this session
    // soc_cc_ is just the dumb uncorrected.
    // they are offsetted by q_offset_as_ / cap_est_as_. so when cap_est_as_ and q_offset_as_ are stable we diverge

    // Coulomb integration (charge positive, discharge negative).
    as_session_ = param::as;
    q_total_ = q_total_init_ + as_session_;

    soc_est_old_ = soc_est_;
    if (cap_est_as_ > 0.0f)
    {
        soc_cc_ = q_total_ / cap_est_as_;
        soc_cc_ = std::clamp(soc_cc_, 0.0f, 1.0f);
        soc_est_ = (q_total_ + q_offset_as_) / cap_est_as_;
        soc_est_ = std::clamp(soc_est_, 0.0f, 1.0f);
    }
    else
    {
        soc_cc_ = 0.0f;
        soc_est_ = 0.0f;
    }

    // Recalibration of the SOC by OCV LUT
    if (std::fabs(param::current) < CC_REST_THRESHOLD_A)
    {
        ocv_rest_timer_ += dt_s;
    }
    else
    {
        ocv_rest_timer_ = 0.0f;
    }

    if (ocv_rest_timer_ > CC_REST_TIME_MIN_S)
    {
        const float soc_hi = SOC_FROM_OCV_TEMP(avg_temp_c, v_max) / 100.0f;
        const float soc_lo = SOC_FROM_OCV_TEMP(avg_temp_c, v_min) / 100.0f;
        float soc_ocv = NAN;

        if (soc_hi > CC_SOC_HIGH_THRESHOLD)
        {
            soc_ocv = soc_hi;
        }
        else if (soc_lo < CC_SOC_LOW_THRESHOLD)
        {
            soc_ocv = soc_lo;
        }

        if (std::isfinite(soc_ocv))
        {
            q_offset_as_ += CC_K_GAIN * (soc_ocv - soc_est_) * cap_est_as_;
        }
    }

    // Todo: we implement a new recalibration algorithm here. we introduce recal_start_soc_ as long as we are below threshold we continue collecting the lowest q_total_ soc_cc pair we can get. As soon as we get above the high threshold, we start calculating the new_cap. recal session should be over, when again below threshold. only then cap_est is taken over with factor alpha

    // Recalibration of the estimated capacity
    if ((soc_est_ > CC_SOC_LOW_THRESHOLD) && (soc_est_old_ < CC_SOC_LOW_THRESHOLD)) // When ever below the threshold collect the latest values. This consideres also correction of the soc
    {
        recal_active_ = true;
        recal_start_q_ = q_total_; // We don't take the offset for calbration, to not calibrate the capacity with offset shift
    }

    // if (soc_est_ > CC_SOC_HIGH_THRESHOLD && recal_active_) // Keep going correcting
    // {
    // }

    if ((soc_est_ < CC_SOC_HIGH_THRESHOLD) && (soc_est_old_ > CC_SOC_HIGH_THRESHOLD) && (recal_active_ == true)) // Calibration is just over, when we hin the high threshold coming from above
    {
        const float delta_soc = soc_est_ - CC_SOC_LOW_THRESHOLD;
        const float delta_q = q_total_ - recal_start_q_;

        const float new_cap = delta_q / delta_soc;
        cap_est_as_ = CC_ALPHA * new_cap +
                      (1.0f - CC_ALPHA) * cap_est_as_;
        soh_ = (CC_CAP_RATED_AS > 0.0f)
                   ? (cap_est_as_ / CC_CAP_RATED_AS)
                   : 0.0f;

        recal_active_ = false;
    }

    publish_params_();
}

void CoulombCounting::publish_params_()
{
    param::cap_est_as = cap_est_as_;
    param::q_offset_as = q_offset_as_;
    param::q_total_init = q_total_init_;
    param::recal_active = recal_active_;
    param::recal_start_q = recal_start_q_;
    param::soh = soh_;
    param::as_session = as_session_;
    param::ocv_rest_timer = ocv_rest_timer_;
    param::soc_cc = soc_cc_;
    param::soc_est = soc_est_;
    param::q_total = q_total_;
    param::coulomb_state = state_;
}
