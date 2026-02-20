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
