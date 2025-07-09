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

static const float kVoltageLevels[31] LUT_PROGMEM = {
  3.390, 3.415, 3.440, 3.465, 3.490, 3.515, 3.540, 3.565, 3.590, 3.615,
  3.640, 3.665, 3.690, 3.715, 3.740, 3.765, 3.790, 3.815, 3.840, 3.865,
  3.890, 3.915, 3.940, 3.965, 3.990, 4.015, 4.040, 4.065, 4.090, 4.115,
  4.140
};

static const float kSocTable[31][4] LUT_PROGMEM = {
  {  0.000,   0.000,   0.000,   0.000},
  {  3.509,   1.279,   0.000,   0.000},
  {  7.895,   4.186,   2.432,   1.571},
  { 11.083,   7.093,   5.811,   5.143},
  { 13.167,  10.000,   9.189,   8.714},
  { 15.250,  12.907,  12.568,  12.286},
  { 17.333,  15.814,  15.946,  15.857},
  { 19.417,  18.721,  19.324,  19.429},
  { 21.500,  21.628,  22.703,  23.000},
  { 23.583,  24.535,  26.081,  26.571},
  { 25.667,  27.442,  29.459,  30.143},
  { 27.750,  30.349,  32.838,  33.714},
  { 29.833,  33.256,  36.216,  37.286},
  { 31.917,  36.163,  39.595,  40.857},
  { 34.000,  39.070,  42.973,  44.429},
  { 36.083,  41.977,  46.351,  48.000},
  { 38.167,  44.884,  49.730,  51.571},
  { 40.250,  47.791,  53.108,  55.143},
  { 42.333,  50.698,  56.486,  58.714},
  { 44.417,  53.605,  59.865,  62.286},
  { 46.500,  56.512,  63.243,  65.857},
  { 48.583,  59.419,  66.622,  69.429},
  { 50.667,  62.326,  70.000,  73.000},
  { 52.750,  65.233,  73.378,  76.571},
  { 54.833,  68.140,  76.757,  80.143},
  { 56.917,  71.047,  80.135,  83.714},
  { 59.000,  73.953,  83.514,  87.286},
  { 61.083,  76.860,  86.892,  90.857},
  { 63.167,  79.767,  90.270,  94.429},
  { 65.250,  82.674,  93.649,  98.000},
  { 67.333,  85.581,  97.027, 100.000}
};

inline float socFromOcvTemp(float temperature, float ocv) {
    static Map3D<31, 4, float, float> map;
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
