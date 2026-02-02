#pragma once

#include <Arduino.h>
#include <cmath>

#include "bms/current.h"

enum class HVMonitorState : uint8_t {
  INIT,
  OPERATING,
  FAULT
};

enum HVMonitorDTC : uint8_t {
  DTC_HVM_NONE = 0,
  DTC_HVM_INVALID_IN_VOLTAGE = 1 << 0,
};

namespace param {
extern bool voltage_matched;
extern HVMonitorState hv_monitor_state;
extern HVMonitorDTC hv_monitor_dtc;
}

class HVMonitor {
public:

#define HV_MONITOR_VOLTAGE_MATCH_TOLLERANCE 5.0f
#define HV_MONITOR_MIN_INPUT_VOLTAGE 10.0f

  HVMonitor() = default;

  void initialise() {
    _state = HVMonitorState::INIT;
    _dtc = DTC_HVM_NONE;
    param::voltage_matched = false;
    param::hv_monitor_state = _state;
    param::hv_monitor_dtc = _dtc;
  }

  void update() {
    if (param::state == ShuntState::FAULT) {
      _state = HVMonitorState::FAULT;
      param::voltage_matched = false;
      return;
    }

    if (param::state == ShuntState::OPERATING &&
        param::u_input_hvbox < HV_MONITOR_MIN_INPUT_VOLTAGE) {
      setDtcFlag_(DTC_HVM_INVALID_IN_VOLTAGE);
      _state = HVMonitorState::FAULT;
      param::voltage_matched = false;
      param::hv_monitor_state = _state;
      return;
    }

    switch (_state) {
      case HVMonitorState::INIT:
        if (param::state == ShuntState::OPERATING) {
          _state = HVMonitorState::OPERATING;
        }
        break;
      case HVMonitorState::OPERATING:
        if (param::state != ShuntState::OPERATING) {
          _state = HVMonitorState::INIT;
        }
        break;
      case HVMonitorState::FAULT:
        break;
    }

    param::hv_monitor_state = _state;

    if (_state == HVMonitorState::OPERATING) {
      const float delta_v =
          std::fabs(param::u_input_hvbox - param::u_output_hvbox);
      param::voltage_matched = (delta_v <= HV_MONITOR_VOLTAGE_MATCH_TOLLERANCE);
    } else {
      param::voltage_matched = false;
    }
  }

  HVMonitorState getState() const { return _state; }
  HVMonitorDTC getDTC() const { return _dtc; }

private:
  void setDtcFlag_(HVMonitorDTC flag) {
    _dtc = static_cast<HVMonitorDTC>(_dtc | flag);
    param::hv_monitor_dtc = _dtc;
  }

  HVMonitorState _state;
  HVMonitorDTC _dtc;
};

extern HVMonitor hv_monitor;
