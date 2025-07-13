#ifndef RESISTANCE_LOOKUP_H
#define RESISTANCE_LOOKUP_H

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "utils/Map2D3D.h"

// temperature_index:
// 0 = -25°C
// 1 = -10°C
// 2 = 25°C
// 3 = 40°C

// current_index:
// 0 = 413A (5s pulse)
// 1 = 294A (30s pulse)

// soc_index:
// 0  = 100%
// 1  = 95%
// 2  = 90%
// ...
// 11 = 5%

#ifdef ARDUINO
#define LUT_PROGMEM PROGMEM
#else
#define LUT_PROGMEM
#endif

static const float kResistanceTemperatureLevels[4] LUT_PROGMEM = {
    -25, -10, 25, 40
};

static const float kResistanceSocLevels[12] LUT_PROGMEM = {
    100, 95, 90, 80, 70, 60, 50, 40, 30, 20, 10, 5
};

static const float kResistanceTable413A[12][4] LUT_PROGMEM = {
    {3.85, 2.22, 0.69, 0.53},
    {3.84, 2.22, 0.68, 0.53},
    {3.87, 2.22, 0.68, 0.52},
    {3.90, 2.25, 0.68, 0.52},
    {4.00, 2.29, 0.68, 0.52},
    {4.14, 2.34, 0.68, 0.52},
    {4.32, 2.40, 0.68, 0.52},
    {5.66, 2.84, 0.68, 0.51},
    {8.41, 3.59, 0.74, 0.52},
    {14.68, 5.20, 0.99, 0.54},
    {18.01, 9.53, 1.81, 0.60},
    {16.31, 11.30, 1.81, 0.71}
};

static const float kResistanceTable294A[12][4] LUT_PROGMEM = {
    {4.29, 2.77, 0.96, 0.75},
    {4.31, 2.77, 0.95, 0.74},
    {4.39, 2.76, 0.94, 0.73},
    {4.42, 2.77, 0.93, 0.72},
    {4.54, 2.81, 0.91, 0.71},
    {4.71, 2.89, 0.91, 0.70},
    {5.09, 2.98, 0.90, 0.67},
    {5.44, 3.24, 0.88, 0.65},
    {10.69, 3.72, 0.91, 0.67},
    {19.61, 6.47, 1.03, 0.75},
    {39.36, 14.63, 1.88, 0.90},
    {48.00, 22.52, 3.79, 1.51}
};

inline float resistanceFromSocTemp413A(float temperature, float soc) {
    static Map3D<12, 4, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setX1s_P(kResistanceSocLevels);
        map.setX2s_P(kResistanceTemperatureLevels);
        map.setYs_P(&kResistanceTable413A[0][0]);
#else
        map.setX1s(kResistanceSocLevels);
        map.setX2s(kResistanceTemperatureLevels);
        map.setYs(&kResistanceTable413A[0][0]);
#endif
        initialized = true;
    }
    return map.f(soc, temperature);
}

inline float resistanceFromSocTemp294A(float temperature, float soc) {
    static Map3D<12, 4, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setX1s_P(kResistanceSocLevels);
        map.setX2s_P(kResistanceTemperatureLevels);
        map.setYs_P(&kResistanceTable294A[0][0]);
#else
        map.setX1s(kResistanceSocLevels);
        map.setX2s(kResistanceTemperatureLevels);
        map.setYs(&kResistanceTable294A[0][0]);
#endif
        initialized = true;
    }
    return map.f(soc, temperature);
}

inline float resistanceFromSocTemp(float temperature, float soc, int current_index) {
    return current_index == 0 ?
        resistanceFromSocTemp413A(temperature, soc) :
        resistanceFromSocTemp294A(temperature, soc);
}

#define RESISTANCE_FROM_SOC_TEMP(t, soc, current) \
    resistanceFromSocTemp((t), (soc), (current))

#endif // RESISTANCE_LOOKUP_H
