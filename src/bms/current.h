#pragma once

#include <Arduino.h>
#include <ACAN_T4.h>  // for CANMessage
#include "settings.h"

enum class ShuntState : uint8_t {
  INIT,
  OPERATING,
  FAULT
};

enum ShuntDTC : uint8_t {
  SHUNT_DTC_NONE = 0,
  SHUNT_DTC_CAN_INIT_ERROR = 1 << 0,
  SHUNT_DTC_TEMPERATURE_TOO_HIGH = 1 << 1,
  SHUNT_DTC_MAX_CURRENT_EXCEEDED = 1 << 2,
  SHUNT_DTC_TIMED_OUT = 1 << 3,
};

namespace param {
extern float current;
extern float u1;
extern float u2;
extern float u3;
extern float temp;
extern float power;
extern float as;
extern float wh;
extern float current_avg;
extern float current_dA_per_s;
extern ShuntState state;
extern ShuntDTC dtc;
}

// -----------------------------------------------------------------------------
//  Single-header IVT-S ISA shunt class
//  - Call initialise() in setup()
//  - Call DecodeCAN(msg) for each received CAN frame
//  - Use getters to read latest values
// -----------------------------------------------------------------------------
class Shunt_IVTS {
public:
  // Basic state
  using STATE = ShuntState;
  using DTC = ShuntDTC;

  // Status nibble (DB1[7:4]) + counter (DB1[3:0]) from result frames
  struct StatusBits {
    bool    ocs = false;                         // open circuit sense
    bool    this_out_of_range_or_me_err = false; // this result out-of-range / meas error
    bool    any_me_err = false;                  // any result has meas error
    bool    system_err = false;                  // system error
    uint8_t counter = 0;                         // cyclic counter 0..15
  };

  // Constructor: set little_endian = true if you configure IVT-S for LE
  explicit Shunt_IVTS(bool little_endian = false)
  : _little_endian(little_endian) {}

  // Call once at startup
  void initialise() {
    _state          = STATE::INIT;
    _last_valid_ms  = 0;
    _first_current  = true;
    _prev_ms        = millis();
    _dtc            = SHUNT_DTC_NONE;

    _cur_A   = 0.0f;
    _u1_V    = 0.0f;
    _u2_V    = 0.0f;
    _u3_V    = 0.0f;
    _temp_C  = 0.0f;
    _power_W = 0.0f;
    _as_As   = 0.0f;
    _wh_Wh   = 0.0f;

    _cur_avg_A     = 0.0f;
    _cur_dA_per_s  = 0.0f;
    _last_cur_A    = 0.0f;

    _st_I  = StatusBits{};
    _st_U1 = StatusBits{};
    _st_U2 = StatusBits{};
    _st_U3 = StatusBits{};
    _st_T  = StatusBits{};
    _st_W  = StatusBits{};
    _st_As = StatusBits{};
    _st_Wh = StatusBits{};

    param::current = 0.0f;
    param::u1 = 0.0f;
    param::u2 = 0.0f;
    param::u3 = 0.0f;
    param::temp = 0.0f;
    param::power = 0.0f;
    param::as = 0.0f;
    param::wh = 0.0f;
    param::current_avg = 0.0f;
    param::current_dA_per_s = 0.0f;
    param::state = _state;
    param::dtc = _dtc;

    Serial.println("IVT-S shunt initialised (CAN handled externally)");
  }

  // ---------------------------------------------------------------------------
  // Call this from your external CAN receive loop for every received message
  // ---------------------------------------------------------------------------
  void DecodeCAN(const CANMessage& m) {
    // Only IVT-S result frames have DLC 6; ignore others quickly
    if (m.len < 6) {
      return;
    }

    // Constants: IVT-S default result CAN IDs (node address 0)
    static constexpr uint32_t ID_I   = 0x521;
    static constexpr uint32_t ID_U1  = 0x522;
    static constexpr uint32_t ID_U2  = 0x523;
    static constexpr uint32_t ID_U3  = 0x524;
    static constexpr uint32_t ID_T   = 0x525;
    static constexpr uint32_t ID_W   = 0x526;
    static constexpr uint32_t ID_As  = 0x527;
    static constexpr uint32_t ID_Wh  = 0x528;

    const uint8_t mux = m.data[0];  // DB0 = mux 0x00..0x07
    const uint8_t b1  = m.data[1];  // DB1 = status + counter

    const int32_t raw = _little_endian
                        ? readS32_le(m.data)
                        : readS32_be(m.data);

    const uint32_t now = millis();

    switch (m.id) {
      case ID_I: { // Current [1 mA / LSB]
        if (mux != 0x00) break;
        _st_I = parseStatus_(b1);
        if (!_st_I.system_err) {
          _cur_A = raw / 1000.0f;
          param::current = _cur_A;
          _last_valid_ms = now;
          if (_state == STATE::INIT) {
            _state = STATE::OPERATING;
            param::state = _state;
          }
          if (_cur_A > BMS_MAX_DISCHARGE_PEAK_CURRENT) {
            setDtcFlag_(SHUNT_DTC_MAX_CURRENT_EXCEEDED);
            _state = STATE::FAULT;
            param::state = _state;
          }
          onCurrentUpdate_(now);
        }
      } break;

      case ID_U1: { // Voltage 1 [1 mV / LSB]
        if (mux != 0x01) break;
        _st_U1 = parseStatus_(b1);
        if (!_st_U1.system_err) {
          _u1_V = raw / 1000.0f;
          param::u1 = _u1_V;
        }
      } break;

      case ID_U2: { // Voltage 2 [1 mV / LSB]
        if (mux != 0x02) break;
        _st_U2 = parseStatus_(b1);
        if (!_st_U2.system_err) {
          _u2_V = raw / 1000.0f;
          param::u2 = _u2_V;
        }
      } break;

      case ID_U3: { // Voltage 3 [1 mV / LSB]
        if (mux != 0x03) break;
        _st_U3 = parseStatus_(b1);
        if (!_st_U3.system_err) {
          _u3_V = raw / 1000.0f;
          param::u3 = _u3_V;
        }
      } break;

      case ID_T: { // Temperature [0.1 C / LSB]
        if (mux != 0x04) break;
        _st_T = parseStatus_(b1);
        if (!_st_T.system_err) {
          _temp_C = raw / 10.0f;
          param::temp = _temp_C;
          if (_temp_C > ISA_SHUNT_MAX_TEMPERATURE) {
            setDtcFlag_(SHUNT_DTC_TEMPERATURE_TOO_HIGH);
            _state = STATE::FAULT;
            param::state = _state;
          }
        }
      } break;

      case ID_W: { // Power [1 W / LSB]
        if (mux != 0x05) break;
        _st_W = parseStatus_(b1);
        if (!_st_W.system_err) {
          _power_W = static_cast<float>(raw);
          param::power = _power_W;
        }
      } break;

      case ID_As: { // Charge [1 As / LSB]
        if (mux != 0x06) break;
        _st_As = parseStatus_(b1);
        if (!_st_As.system_err) {
          _as_As = static_cast<float>(raw);
          param::as = _as_As;
        }
      } break;

      case ID_Wh: { // Energy [1 Wh / LSB]
        if (mux != 0x07) break;
        _st_Wh = parseStatus_(b1);
        if (!_st_Wh.system_err) {
          _wh_Wh = static_cast<float>(raw);
          param::wh = _wh_Wh;
        }
      } break;

      default:
        // Ignore non-IVT-S messages
        break;
    }
  }

  // ---------------------------------------------------------------------------
  // Getters
  // ---------------------------------------------------------------------------
  STATE state() const { return _state; }
  DTC dtc() const { return _dtc; }

  // Main measured values
  float current_A()   const { return _cur_A;   }
  float u1_V()        const { return _u1_V;    }
  float u2_V()        const { return _u2_V;    }
  float u3_V()        const { return _u3_V;    }
  float temp_C()      const { return _temp_C;  }
  float power_W()     const { return _power_W; }
  float as_As()       const { return _as_As;   } // current counter from IVT-S
  float wh_Wh()       const { return _wh_Wh;   } // energy counter from IVT-S

  // Filtered current + derivative (your convenience values)
  float current_avg_A()    const { return _cur_avg_A;    }
  float current_dA_per_s() const { return _cur_dA_per_s; }

  // Status per channel
  StatusBits status_I()  const { return _st_I;  }
  StatusBits status_U1() const { return _st_U1; }
  StatusBits status_U2() const { return _st_U2; }
  StatusBits status_U3() const { return _st_U3; }
  StatusBits status_T()  const { return _st_T;  }
  StatusBits status_W()  const { return _st_W;  }
  StatusBits status_As() const { return _st_As; }
  StatusBits status_Wh() const { return _st_Wh; }

  // Optionally call this from your main loop to implement a timeout fault.
  // If now - last_valid_ms > timeout_ms and state == OPERATING => FAULT.
  void checkTimeout(uint32_t timeout_ms) {
    if (_state == STATE::OPERATING && timeout_ms > 0) {
      uint32_t now = millis();
      if ((_last_valid_ms > 0) && (now - _last_valid_ms > timeout_ms)) {
        setDtcFlag_(SHUNT_DTC_TIMED_OUT);
        _state = STATE::FAULT;
        param::state = _state;
      }
    }
  }

  void setDtcFlag(ShuntDTC flag) {
    setDtcFlag_(flag);
  }

private:
  // Read 32-bit signed value from data[2..5] in Big-Endian
  static int32_t readS32_be(const uint8_t* d) {
    return static_cast<int32_t>(
        (static_cast<uint32_t>(d[2]) << 24) |
        (static_cast<uint32_t>(d[3]) << 16) |
        (static_cast<uint32_t>(d[4]) << 8)  |
        (static_cast<uint32_t>(d[5]))
    );
  }

  // Read 32-bit signed value from data[2..5] in Little-Endian
  static int32_t readS32_le(const uint8_t* d) {
    return static_cast<int32_t>(
        (static_cast<uint32_t>(d[5]) << 24) |
        (static_cast<uint32_t>(d[4]) << 16) |
        (static_cast<uint32_t>(d[3]) << 8)  |
        (static_cast<uint32_t>(d[2]))
    );
  }

  // Parse DB1 into status bits
  static StatusBits parseStatus_(uint8_t b1) {
    StatusBits s;
    s.counter = static_cast<uint8_t>(b1 & 0x0F);
    uint8_t hi = static_cast<uint8_t>(b1 >> 4);
    s.ocs                           = (hi & 0x01) != 0;
    s.this_out_of_range_or_me_err   = (hi & 0x02) != 0;
    s.any_me_err                    = (hi & 0x04) != 0;
    s.system_err                    = (hi & 0x08) != 0;
    return s;
  }

  // Called whenever we get a new current value
  void onCurrentUpdate_(uint32_t now_ms) {
    uint32_t dt_ms = now_ms - _prev_ms;
    _prev_ms = now_ms;

    if (_first_current) {
      _first_current = false;
      _last_cur_A    = _cur_A;
      _cur_avg_A     = _cur_A;
      _cur_dA_per_s  = 0.0f;
      param::current_avg = _cur_avg_A;
      param::current_dA_per_s = _cur_dA_per_s;
      return;
    }

    // Avoid divide-by-zero; treat extremely small dt as 1ms
    if (dt_ms == 0) dt_ms = 1;

    const float dt_s = dt_ms / 1000.0f;

    // Exponential moving average with ~1s time constant
    float alpha = dt_s / 1.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    _cur_avg_A = alpha * _cur_A + (1.0f - alpha) * _last_cur_A;

    // Derivative (A/s)
    _cur_dA_per_s = (_cur_A - _last_cur_A) / dt_s;

    _last_cur_A = _cur_A;
    param::current_avg = _cur_avg_A;
    param::current_dA_per_s = _cur_dA_per_s;
  }

  void setDtcFlag_(ShuntDTC flag) {
    _dtc = static_cast<ShuntDTC>(_dtc | flag);
    param::dtc = _dtc;
  }

private:
  // Configuration
  bool _little_endian = false;

  // State
  STATE    _state         = STATE::INIT;
  uint32_t _last_valid_ms = 0;
  ShuntDTC _dtc           = SHUNT_DTC_NONE;

  // Measurements (latest)
  float _cur_A   = 0.0f;
  float _u1_V    = 0.0f;
  float _u2_V    = 0.0f;
  float _u3_V    = 0.0f;
  float _temp_C  = 0.0f;
  float _power_W = 0.0f;
  float _as_As   = 0.0f;
  float _wh_Wh   = 0.0f;

  // Filtered / derived values
  float    _cur_avg_A    = 0.0f;
  float    _cur_dA_per_s = 0.0f;
  float    _last_cur_A   = 0.0f;
  uint32_t _prev_ms      = 0;
  bool     _first_current = true;

  // Last status of each result channel
  StatusBits _st_I;
  StatusBits _st_U1;
  StatusBits _st_U2;
  StatusBits _st_U3;
  StatusBits _st_T;
  StatusBits _st_W;
  StatusBits _st_As;
  StatusBits _st_Wh;
};
