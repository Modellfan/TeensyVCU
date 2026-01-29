#pragma once

#include <Arduino.h>
#include <cmath>

#include "bms/current.h"
#include "settings.h"

// Enhanced Coulomb Counting constants
#define CC_REST_THRESHOLD_A 10.0f
#define CC_REST_TIME_MIN_S 15.0f
#define CC_CAP_RATED_AS (94.0f * 3600.0f)
#define BMS_INITIAL_CAPACITY_AS (BMS_INITIAL_CAPACITY_AH * 3600.0f)

enum class CoulombCountingState : uint8_t {
    INIT,
    OPERATING,
    FAULT
};

namespace param {
extern float b_as;
extern float C_as;
extern float soh;
extern bool have_low_anchor;
extern float q_low_as;
extern float soc_low_anchor;
extern bool was_above_high_set;
extern float q_as;
extern bool ocv_valid;
extern float soc_ocv;
extern float soc_cc;
extern CoulombCountingState coulomb_state;
}

class CoulombCounting {
public:
    CoulombCounting() = default;

    void initialise(float b_as,
                    float C_as,
                    float soh,
                    bool have_low_anchor,
                    float q_low_as,
                    float soc_low_anchor,
                    bool was_above_high_set);

    void update(float v_min, float v_max, float avg_temp_c);

    float soc_cc() const { return soc_cc_; }
    float b_as() const { return b_as_; }
    float C_as() const { return C_as_; }
    float soh() const { return soh_; }
    bool have_low_anchor() const { return have_low_anchor_; }
    float q_low_as() const { return q_low_as_; }
    float soc_low_anchor() const { return soc_low_anchor_; }
    bool was_above_high_set() const { return was_above_high_set_; }
    CoulombCountingState state() const { return state_; }

private:
    void publish_params_();

    float b_as_ = 0.0f;
    float C_as_ = BMS_INITIAL_CAPACITY_AS;
    float soh_ = 1.0f;
    bool have_low_anchor_ = false;
    float q_low_as_ = 0.0f;
    float soc_low_anchor_ = 0.0f;
    bool was_above_high_set_ = false;
    float q_as_ = 0.0f;
    bool ocv_valid_ = false;
    float soc_ocv_ = 0.0f;
    float soc_cc_ = 0.0f;
    float ocv_rest_timer_ = 0.0f;
    uint32_t last_update_ms_ = 0U;
    CoulombCountingState state_ = CoulombCountingState::INIT;
    bool persistent_loaded_ = false;
};
