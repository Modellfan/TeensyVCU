#pragma once

#include <Arduino.h>
#include <cmath>

#include "bms/current.h"
#include "settings.h"

// Enhanced Coulomb Counting constants
#define CC_K_GAIN 0.1f
#define CC_REST_THRESHOLD_A 10.0f
#define CC_REST_TIME_MIN_S 15.0f
#define CC_SOC_HIGH_THRESHOLD 0.80f
#define CC_SOC_LOW_THRESHOLD 0.20f
#define CC_ALPHA 0.5f
#define CC_CAP_RATED_AS (94.0f * 3600.0f)
#define BMS_INITIAL_CAPACITY_AS (BMS_INITIAL_CAPACITY_AH * 3600.0f)

enum class CoulombCountingState : uint8_t {
    INIT,
    OPERATING,
    FAULT
};

namespace param {
extern float cap_est_as;
extern float q_offset_as;
extern float q_total_init;
extern bool recal_active;
extern float recal_start_q;
extern float soh;
extern float as_session;
extern float ocv_rest_timer;
extern float soc_cc;
extern float soc_est;
extern float q_total;
extern CoulombCountingState coulomb_state;
}

class CoulombCounting {
public:
    CoulombCounting() = default;

    void initialise(float cap_est_as,
                    float q_total_init,
                    bool recal_active,
                    float recal_start_q);

    void update(float v_min, float v_max, float avg_temp_c);

    float soc_est() const { return soc_est_; }
    float cap_est_as() const { return cap_est_as_; }
    float q_offset_as() const { return q_offset_as_; }
    float q_total_init() const { return q_total_init_; }
    bool recal_active() const { return recal_active_; }
    float recal_start_q() const { return recal_start_q_; }
    float soh() const { return soh_; }
    float as_session() const { return as_session_; }
    float ocv_rest_timer() const { return ocv_rest_timer_; }
    float q_total() const { return q_total_; }
    CoulombCountingState state() const { return state_; }

private:
    void publish_params_();

    float as_session_ = 0.0f;
    float ocv_rest_timer_ = 0.0f;
    float soc_cc_ = 0.0f;
    float soc_est_ = 0.0f;
    float cap_est_as_ = BMS_INITIAL_CAPACITY_AS;
    float q_offset_as_ = 0.0f;
    float q_total_init_ = 0.0f;
    bool recal_active_ = false;
    float recal_start_q_ = 0.0f;
    float soh_ = 1.0f;
    float q_total_ = 0.0f;
    uint32_t last_update_ms_ = 0U;
    CoulombCountingState state_ = CoulombCountingState::INIT;
    bool persistent_loaded_ = false;
};
