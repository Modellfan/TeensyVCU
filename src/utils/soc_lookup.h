#ifndef SOC_LOOKUP_H
#define SOC_LOOKUP_H

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "utils/Map2D3D.h"

#ifdef ARDUINO
#define LUT_PROGMEM PROGMEM
#else
#define LUT_PROGMEM
#endif

static const float kTemperatureLevels[4] LUT_PROGMEM = {
  40, 25, -10, -25
};

static const float kVoltageLevels[17] LUT_PROGMEM = {
  3.350, 3.400, 3.450, 3.500, 3.550, 3.600, 3.650, 3.700, 3.750, 3.800,
  3.850, 3.900, 3.950, 4.000, 4.050, 4.100, 4.150
};

static const float kSocTable[17][4] LUT_PROGMEM = {
  {  0.000,   0.000,   0.000,   0.000},
  {  0.880,   0.000,   0.000,   0.000},
  {  9.650,   5.350,   3.780,   3.000},
  { 14.000,  11.190,  10.480,  10.120},
  { 18.170,  17.140,  16.510,  16.220},
  { 26.360,  26.500,  26.560,  26.330},
  { 42.500,  42.570,  44.320,  41.360},
  { 53.150,  53.200,  53.630,  53.730},
  { 60.000,  59.870,  59.880,  59.760},
  { 67.350,  67.210,  67.100,  66.960},
  { 73.570,  73.520,  73.370,  73.260},
  { 79.150,  79.200,  78.990,  78.880},
  { 84.180,  84.170,  83.980,  83.880},
  { 89.110,  89.030,  88.830,  88.740},
  { 93.420,  93.360,  93.190,  93.110},
  { 97.580,  97.560,  97.390,  97.310},
  {100.000, 100.000, 100.000, 100.000}
};

inline float socFromOcvTemp(float temperature, float ocv) {
    static Map3D<17, 4, float, float> map;
    static bool initialized = false;
    if (!initialized) {
#ifdef ARDUINO
        map.setX1s_P(kVoltageLevels);
        map.setX2s_P(kTemperatureLevels);
        map.setYs_P(&kSocTable[0][0]);
#else
        map.setX1s(kVoltageLevels);
        map.setX2s(kTemperatureLevels);
        map.setYs(&kSocTable[0][0]);
#endif
        initialized = true;
    }
    return map.f(ocv, temperature);
}

#define SOC_FROM_OCV_TEMP(t, ocv) socFromOcvTemp((t), (ocv))

#endif // SOC_LOOKUP_H
