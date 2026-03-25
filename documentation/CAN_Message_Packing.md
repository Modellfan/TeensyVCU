# Final CAN Message Packing (8 bytes, Classical CAN)

The following tables describe the structure of all CAN messages exchanged between the BMS and VCU.

Sign convention used by the BMS on CAN:
- Current and power are positive for discharge and negative for charge.

## MSG1: `BMS_VOLTAGE` (0x41A, 10 Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | Pack Voltage | `uint16` | V x 10 | 0-6500 | 0-650.0 V, 0.1 V resolution |
| 2-3 | Pack Current | `uint16` | (A x 10) + 5000 | 0-10000 | -500 A to +500 A (positive = discharge, negative = charge) |
| 4 | Min Cell Voltage | `uint8` | V x 50 | 0-255 | 0-5.10 V |
| 5 | Max Cell Voltage | `uint8` | V x 50 | 0-255 | 0-5.10 V |
| 6 | Counter (4-bit) | `uint8` | lower 4 bits only | 0-15 | bits 7-4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG2: `BMS_CELL_TEMP` (0x41B, 10 Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0 | Min Cell Temp | `uint8` | (degC) + 40 | 0-167 | -40 degC to +127 degC |
| 1 | Max Cell Temp | `uint8` | (degC) + 40 | 0-167 | -40 degC to +127 degC |
| 2 | Balancing Target Voltage | `uint8` | V x 50 | 0-255 | 0-5.10 V, 0 V when balancing inactive |
| 3 | Cell Voltage Delta | `uint8` | V x 500 | 0-255 | 0-0.51 V (2 mV resolution; highest minus lowest cell) |
| 4-5 | Pack Power | `uint16` | (kW x 100) + 30000 | 0-50000 | -300 kW to +200 kW (positive = discharge, negative = charge) |
| 6 | Counter (4-bit) | `uint8` | lower 4 bits only | 0-15 | bits 7-4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG3: `BMS_LIMITS_FAULT` (0x41C, 10 Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | Max Discharge Current | `uint16` | A x 10 | 0-65535 | 0-6553.5 A (magnitude, always positive) |
| 2-3 | Max Charge Current | `uint16` | A x 10 | 0-65535 | 0-6553.5 A (magnitude, always positive) |
| 4 | Contactor State | `uint8` | enum | 0-7 | 0=INIT,1=OPEN,2=CLOSING_PRE,3=CLOSING_POS,4=CLOSED,5=OPENING_POS,6=OPENING_PRE,7=FAULT (state only) |
| 5 | Fault Code | `uint8` | bitfield | 0-255 | BMS DTC flags (`BMS::DTC_BMS`) |
| 6 | Counter (4-bit) | `uint8` | lower 4 bits only | 0-15 | bits 7-4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

Current firmware transmits `BMS::dtc` in MSG3 byte 5. The bit mapping is:

| Bit | Name | Description |
|-----|------|-------------|
| 0 | `DTC_BMS_CAN_SEND_ERROR` | A CAN transmit error occurred. |
| 1 | `DTC_BMS_CAN_INIT_ERROR` | BMS CAN initialization failed. |
| 2 | `DTC_BMS_PACK_FAULT` | Battery pack reported fault. |
| 3 | `DTC_BMS_CONTACTOR_FAULT` | Contactor manager entered FAULT state. |
| 4 | `DTC_BMS_SHUNT_FAULT` | Shunt/current measurement reported fault. |
| 5-7 | (reserved) | Unused; transmit as 0. |

Notes:
- Byte 4 value `7` means the contactor manager state machine is in `FAULT`.
- Byte 5 bit `0x08` (`DTC_BMS_CONTACTOR_FAULT`) is a BMS-level fault flag and is not a per-contactor diagnostic bit.

## MSG4: `BMS_SOC_SOH` (0x41D, 1 Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | SOC | `uint16` | % x 100 | 0-10000 | 0-100.00 % |
| 2-3 | SOH | `uint16` | % x 100 | 0-10000 | 0-100.00 % |
| 4 | Balancing Status | `uint8` | enum | 0-2 | 0=idle, 1=balancing, 2=finished |
| 5 | BMS Status | `uint8` | enum | 0-2 | 0=INIT, 1=OPERATING, 2=FAULT |
| 6 | Counter (4-bit) | `uint8` | lower 4 bits only | 0-15 | bits 7-4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG5: `BMS_HMI` (0x41E, 1 Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | Averaged Energy per Hour | `int16` | kWh x 100 | -32768-32767 | sign indicates charge (-) or discharge (+) |
| 2-3 | Remaining Time | `uint16` | seconds | 0-65535 | time to empty when discharging, time to full when charging |
| 4-5 | Remaining Energy | `uint16` | Wh | 0-65535 | energy left |
| 6 | Counter (4-bit) | `uint8` | lower 4 bits only | 0-15 | bits 7-4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## Extra section: explicit contactor telemetry mirrored from serial console (`BMS_CONTACTOR_TELEMETRY`, 0x601, 10 Hz)

This message explicitly publishes the same contactor values printed by serial console command `s` (`print_contactor_status()`):
- Manager state
- Manager DTC
- Precharge strategy
- Negative contactor closed
- Contactor supply available
- Positive contactor state
- Positive contactor feedback
- Positive contactor DTC
- Precharge contactor state
- Precharge contactor feedback
- Precharge contactor DTC
- Positive contactor can open (current-dependent)

Enums are byte-aligned (not sub-byte packed). Bools are packed into a single
bitfield byte. This telemetry frame intentionally has no alive counter and no
CRC byte.

| Byte | Signal | Type | Encoding |
|-----|--------|------|----------|
| 0 | Manager state | enum (`uint8`) | `Contactormanager::State` (`0..7`) |
| 1 | Manager DTC | bitfield (`uint8`) | `Contactormanager::DTC_COM` |
| 2 | Precharge strategy | enum (`uint8`) | `0=TIMED_DELAY`, `1=VOLTAGE_MATCH` |
| 3 | Positive contactor state | enum (`uint8`) | `Contactor::State` (`0..5`) |
| 4 | Positive contactor DTC | bitfield (`uint8`) | `Contactor::DTC_CON` |
| 5 | Precharge contactor state | enum (`uint8`) | `Contactor::State` (`0..5`) |
| 6 | Precharge contactor DTC | bitfield (`uint8`) | `Contactor::DTC_CON` |
| 7 bit0 | Negative contactor closed | bool | `0=false`, `1=true` |
| 7 bit1 | Contactor supply available | bool | `0=false`, `1=true` |
| 7 bit2 | Positive contactor feedback | bool | `0=open`, `1=closed` |
| 7 bit3 | Precharge contactor feedback | bool | `0=open`, `1=closed` |
| 7 bit4 | Positive contactor can open | bool | `0=no`, `1=yes` |
| 7 bit7:5 | reserved |  | transmit `0` |


## Extra section: shunt + HV monitor telemetry (`0x602`-`0x605`, 10 Hz)

These frames mirror all currently available shunt and HV monitor runtime telemetry
(`param::*`) in the same style as contactor telemetry: fixed CAN IDs and raw
telemetry payload bytes without CRC/counter fields. All numeric values are packed
as `uint16` with defined scale/offset.
Values that exceed the representable range are saturated to `0..65535`.

### `BMS_SHUNT_HV_META` (0x602)

| Byte | Signal | Type | Encoding |
|-----|--------|------|----------|
| 0 | Shunt state | enum (`uint8`) | `ShuntState` (`0=INIT,1=OPERATING,2=FAULT`) |
| 1 | Shunt DTC LSB | `uint8` | `param::dtc` bits 7:0 |
| 2 | Shunt DTC MSB | `uint8` | `param::dtc` bits 15:8 |
| 3 | HV monitor state | enum (`uint8`) | `HVMonitorState` (`0=INIT,1=OPERATING,2=FAULT`) |
| 4 | HV monitor DTC | bitfield (`uint8`) | `HVMonitorDTC` |
| 5 | Voltage matched | bool (`uint8`) | `0=false`, `1=true` |
| 6-7 | HV monitor delta voltage | `uint16` | `V x 100` |

### `BMS_MSG_SHUNT_HV_I_CURRENT_COUNTERS` (0x603)

| Byte | Signal | Type | Scaling/Offset |
|-----|--------|------|----------------|
| 0-1 | Current | `uint16` | `(A x 10) + 5000` |
| 2-3 | Current average | `uint16` | `(A x 10) + 5000` |
| 4-5 | Charge counter (`As`) | `uint16` | `As + 32768` |
| 6-7 | Energy counter (`Wh`) | `uint16` | `Wh + 32768` |

### `BMS_MSG_SHUNT_HV_DI_TEMP_POWER` (0x604)

| Byte | Signal | Type | Scaling/Offset |
|-----|--------|------|----------------|
| 0-1 | Current derivative | `uint16` | `(A/s x 1) + 32768` |
| 2-3 | Temperature | `uint16` | `(degC x 10) + 400` |
| 4-5 | Power | `uint16` | `(W x 0.1) + 32768` |
| 6-7 | reserved | `uint16` | `0` |

### `BMS_MSG_SHUNT_HV_U12_U3` (0x605)

| Byte | Signal | Type | Scaling/Offset |
|-----|--------|------|----------------|
| 0-1 | Input HV box voltage (`U1`) | `uint16` | `V x 100` |
| 2-3 | Output HV box voltage (`U2`) | `uint16` | `V x 100` |
| 4-5 | `U3` voltage | `uint16` | `V x 100` |
| 6-7 | reserved | `uint16` | `0` |

## VCU to BMS (`BMS_VCU`, 0x437)

All signals below are unsigned unless otherwise noted.

| Byte | Signal | Type | Value/Range | Notes |
|-----|--------|------|-------------|-------|
| 0 | VehicleOperatingMode | `uint8` | enum |  |
| 1 | RequestBMSShutdown | `uint8` | 0/1 | boolean as `uint8` |
| 2 | RequestContactorClose | `uint8` | 0/1 | boolean as `uint8` |
| 3 | reserved |  |  |  |
| 4-5 | reserved |  |  |  |
| 6 | Counter (4-bit) | `uint8` | lower 4 bits only | bits 7-4 always 0 |
| 7 | CRC8 | `uint8` |  |  |

`VehicleOperatingMode` matches `BMS::VehicleState` (byte values shown):

| Value | Name |
|-------|------|
| 255 | `STATE_INVALID` (-1) |
| 0 | `STATE_SLEEP` |
| 1 | `STATE_STANDBY` |
| 2 | `STATE_HV_CONNECTING` |
| 3 | `STATE_HV_DISCONNECTING` |
| 4 | `STATE_READY` |
| 5 | `STATE_CONDITIONING` |
| 6 | `STATE_DRIVE` |
| 7 | `STATE_CHARGE` |
| 8 | `STATE_ERROR` |
| 9 | `STATE_LIMP_HOME` |
