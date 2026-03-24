#include "bms/hv_monitor.h"

namespace param {
bool voltage_matched = false;
float hv_monitor_delta_voltage = 0.0f;
HVMonitorState hv_monitor_state = HVMonitorState::INIT;
HVMonitorDTC hv_monitor_dtc = DTC_HVM_NONE;
}
