#ifndef CURRENT_LIMIT_LOOKUP_H
#define CURRENT_LIMIT_LOOKUP_H

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "utils/Map2D3D.h"

#ifdef ARDUINO
#define LUT_PROGMEM PROGMEM
#else
#define LUT_PROGMEM
#endif

static const float kCurrentLimitTemps[18] LUT_PROGMEM = {
  60, 50, 40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -40
};

static const float kChargeCurrentLimitPeak[18] LUT_PROGMEM = {
  270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 237, 185, 125, 62, 33, 22, 10, 1
};

static const float kChargeCurrentLimitContinuous[18] LUT_PROGMEM = {
  107, 107, 96, 84, 73, 61, 51, 41, 32, 24, 18, 12, 7.2, 4.3, 2.7, 1.7, 1.0, 0.4
};

static const float kDischargeCurrentLimitPeak[18] LUT_PROGMEM = {
  409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409
};

static const float kDischargeCurrentLimitContinuous[18] LUT_PROGMEM = {
  223, 223, 223, 210, 196, 180, 166, 153, 136, 124, 108, 93, 77, 74, 62, 57, 46, 33
};

inline float chargePeakCurrentLimit(float temperature) {
    static Map2D<18, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setXs_P(kCurrentLimitTemps);
        map.setYs_P(kChargeCurrentLimitPeak);
#else
        map.setXs(kCurrentLimitTemps);
        map.setYs(kChargeCurrentLimitPeak);
#endif
        initialized = true;
    }
    return map.f(temperature);
}

inline float chargeContinuousCurrentLimit(float temperature) {
    static Map2D<18, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setXs_P(kCurrentLimitTemps);
        map.setYs_P(kChargeCurrentLimitContinuous);
#else
        map.setXs(kCurrentLimitTemps);
        map.setYs(kChargeCurrentLimitContinuous);
#endif
        initialized = true;
    }
    return map.f(temperature);
}

inline float dischargePeakCurrentLimit(float temperature) {
    static Map2D<18, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setXs_P(kCurrentLimitTemps);
        map.setYs_P(kDischargeCurrentLimitPeak);
#else
        map.setXs(kCurrentLimitTemps);
        map.setYs(kDischargeCurrentLimitPeak);
#endif
        initialized = true;
    }
    return map.f(temperature);
}

inline float dischargeContinuousCurrentLimit(float temperature) {
    static Map2D<18, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setXs_P(kCurrentLimitTemps);
        map.setYs_P(kDischargeCurrentLimitContinuous);
#else
        map.setXs(kCurrentLimitTemps);
        map.setYs(kDischargeCurrentLimitContinuous);
#endif
        initialized = true;
    }
    return map.f(temperature);
}

#define CHARGE_PEAK_CURRENT_LIMIT(t) chargePeakCurrentLimit(t)
#define CHARGE_CONT_CURRENT_LIMIT(t) chargeContinuousCurrentLimit(t)
#define DISCHARGE_PEAK_CURRENT_LIMIT(t) dischargePeakCurrentLimit(t)
#define DISCHARGE_CONT_CURRENT_LIMIT(t) dischargeContinuousCurrentLimit(t)

#endif // CURRENT_LIMIT_LOOKUP_H
