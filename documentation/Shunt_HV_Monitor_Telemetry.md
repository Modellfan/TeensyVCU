# Proposed Shunt and HV Monitor Telemetry Values

This document lists all currently available shunt and HV monitor telemetry values in firmware, including variable type, plausible range, and unit.

## Scope

- **Shunt telemetry source:** `param` variables updated from IVT-S CAN result frames.
- **HV monitor telemetry source:** `param` values derived from shunt voltages and shunt state.
- **Serial monitor mapping:**
  - `i` prints shunt telemetry.
  - `B` prints HV monitor telemetry.

---

## HV Monitor Telemetry

| Telemetry value | Variable | Type | Plausible range | Unit | Serial output |
|---|---|---|---|---|---|
| Monitor state | `param::hv_monitor_state` | `HVMonitorState` (`uint8_t` enum) | `INIT`, `OPERATING`, `FAULT` | N/A | `B` |
| Monitor DTC flags | `param::hv_monitor_dtc` | `HVMonitorDTC` (`uint8_t` bitmask) | `0..255` (`DTC_HVM_INVALID_IN_VOLTAGE` currently used) | N/A | Not currently printed |
| Input/output voltage match | `param::voltage_matched` | `bool` | `0` or `1` | N/A | `B` |

### HV monitor thresholds/logic context

- Minimum valid input voltage while shunt is operating: `10.0 V`.
- Voltage match tolerance: `<= 5.0 V` absolute delta between `U_in` and `U_out`.
- If shunt enters `FAULT`, HV monitor enters `FAULT`.

---

## Shunt Telemetry

| Telemetry value | Variable | Type | Plausible range | Unit | Serial output |
|---|---|---|---|---|---|
| State | `param::state` | `ShuntState` (`uint8_t` enum) | `INIT`, `OPERATING`, `FAULT` | N/A | `i` |
| DTC flags | `param::dtc` | `ShuntDTC` (`uint16_t` bitmask) | `0..65535` | N/A | `i` |
| Current | `param::current` | `float` | Theoretical decode: `~±2,147,483.648`; practical operation is lower; over-current fault at `|I| > 409 A` | A | `i` |
| Input HV box voltage (U1) | `param::u_input_hvbox` | `float` | Theoretical decode: `~±2,147,483.647`; practical operation is pack-voltage domain | V | `i` |
| Output HV box voltage (U2) | `param::u_output_hvbox` | `float` | Theoretical decode: `~±2,147,483.647`; practical operation is pack-voltage domain | V | `i` |
| U3 voltage | `param::u3` | `float` | Theoretical decode: `~±2,147,483.647`; practical operation depends on wiring/use | V | `i` |
| Temperature | `param::temp` | `float` | Theoretical decode: `~±214,748,364.7`; over-temperature fault at `> 70 °C` | °C | `i` |
| Power | `param::power` | `float` | Theoretical decode: `~±2,147,483,647` | W | `i` |
| Charge counter | `param::as` | `float` | Relative software counter (from signed 32-bit source), very large span | As | `i` |
| Energy counter | `param::wh` | `float` | Relative software counter (from signed 32-bit source), very large span | Wh | `i` |
| Filtered current average | `param::current_avg` | `float` | Similar practical range as current | A | `i` |
| Current derivative | `param::current_dA_per_s` | `float` | Unbounded mathematically (depends on sample time and current step) | A/s | `i` |

### Shunt decode scaling

- Current: `1 mA / LSB`
- U1/U2/U3 voltage: `1 mV / LSB`
- Temperature: `0.1 °C / LSB`
- Power: `1 W / LSB`
- Charge: `1 As / LSB`
- Energy: `1 Wh / LSB`

---

## Serial Monitoring Comparison

### `i` command (current sensor status)
Prints:
- state, DTC
- current, current average, dI/dt
- U1/U2/U3
- temperature, power
- charge (As), energy (Wh)
- status bits per result channel (I, U1, U2, U3, T, W, As, Wh)

### `B` command (BMS status)
In the HV Monitor section, prints:
- monitor state
- voltage matched flag

### Current visibility gap
- `param::hv_monitor_dtc` exists internally but is not currently printed by the serial monitor.

### CAN telemetry availability

All values listed in this document are now exported on BMS CAN via dedicated telemetry
messages (`0x602`-`0x605`) alongside contactor telemetry (`0x601`).

- `0x602` publishes shunt/HV monitor states and DTC bitfields.
- `0x603`..`0x605` publish scaled/offset `uint16` telemetry values.

See `documentation/CAN_Message_Packing.md` for exact byte-level layout.
