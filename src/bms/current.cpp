#include "bms/current.h"

namespace param {
float current = 0.0f;
float u1 = 0.0f;
float u2 = 0.0f;
float u3 = 0.0f;
float temp = 0.0f;
float power = 0.0f;
float as = 0.0f;
float wh = 0.0f;
float current_avg = 0.0f;
float current_dA_per_s = 0.0f;
ShuntState state = ShuntState::INIT;
ShuntDTC dtc = SHUNT_DTC_NONE;
}
