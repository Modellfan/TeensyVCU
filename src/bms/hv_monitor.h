#pragma once

#include <Arduino.h>
#include <cmath>

#include "bms/current.h"

enum class HVMonitorState : uint8_t {
  INIT,
  OPERATING,
  FAULT
};

namespace param {
extern bool voltage_matched;
extern HVMonitorState hv_monitor_state;
}

class HVMonitor {
public:

#define HV_MONITOR_VOLTAGE_MATCH_TOLLERANCE 5.0f

  HVMonitor() = default;

  void initialise() {
    _state = HVMonitorState::INIT;
    param::voltage_matched = false;
    param::hv_monitor_state = _state;
  }

  void update() {
    if (param::state == ShuntState::FAULT) {
      _state = HVMonitorState::FAULT;
      param::voltage_matched = false;
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

private:
  HVMonitorState _state;
};

extern HVMonitor hv_monitor;
