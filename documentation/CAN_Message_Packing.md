# Final CAN Message Packing (8 bytes, Classical CAN)

The following tables describe the structure of all CAN messages exchanged between the BMS and VCU.

## MSG1: `BMS_VOLTAGE` (10&nbsp;Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | Pack Voltage | `uint16` | V × 10 | 0–6500 | 0–650.0 V, 0.1 V resolution |
| 2-3 | Pack Current | `uint16` | (A × 10) + 5000 | 0–10000 | –500 A to +500 A |
| 4 | Min Cell Voltage | `uint8` | V × 50 | 0–255 | 0–5.10 V |
| 5 | Max Cell Voltage | `uint8` | V × 50 | 0–255 | 0–5.10 V |
| 6 | Counter (4&nbsp;bit) | `uint8` | lower 4 bits only | 0–15 | bits 7–4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG2: `BMS_CELL_TEMP` (10&nbsp;Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0 | Min Cell Temp | `uint8` | (°C) + 40 | 0–167 | –40 °C to +127 °C |
| 1 | Max Cell Temp | `uint8` | (°C) + 40 | 0–167 | –40 °C to +127 °C |
| 2 | Cell Voltage Avg | `uint8` | V × 50 | 0–255 | 0–5.10 V |
| 3 | Cell Voltage Delta | `uint8` | V × 100 | 0–255 | 0–2.55 V |
| 4-5 | Pack Power | `uint16` | (kW × 10) + 2000 | 0–65535 | –200 kW to +4553 kW |
| 6 | Counter (4&nbsp;bit) | `uint8` | lower 4 bits only | 0–15 | bits 7–4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG3: `BMS_LIMITS_FAULT` (10&nbsp;Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | Max Discharge Current | `uint16` | A × 10 | 0–65535 | 0–6553.5 A |
| 2-3 | Max Charge Current | `uint16` | A × 10 | 0–65535 | 0–6553.5 A |
| 4 | Contactor State | `uint8` | enum | 0–7 | 0=INIT,1=OPEN,2=CLOSING_PRE,3=CLOSING_POS,4=CLOSED,5=OPENING_POS,6=OPENING_PRE,7=FAULT |
| 5 | Fault Code | `uint8` |  | 0–255 | enum |
| 6 | Counter (4&nbsp;bit) | `uint8` | lower 4 bits only | 0–15 | bits 7–4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG4: `BMS_SOC_SOH` (1&nbsp;Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | SOC | `uint16` | % × 100 | 0–10000 | 0–100.00 % |
| 2-3 | SOH | `uint16` | % × 100 | 0–10000 | 0–100.00 % |
| 4 | Balancing Status | `uint8` | enum |  | 0=idle, 1=active, etc. |
| 5 | BMS Status | `uint8` | enum |  | 0=Active, etc. |
| 6 | Counter (4&nbsp;bit) | `uint8` | lower 4 bits only | 0–15 | bits 7–4 always 0 |
| 7 | CRC8 | `uint8` |  |  |  |

## MSG5: `BMS_HMI` (1&nbsp;Hz)

| Byte | Signal | Type | Scaling/Offset | Range | Notes |
|-----|--------|------|---------------|-------|-------|
| 0-1 | Averaged Energy per Hour | `uint16` | kWh × 100 | 0–65535 | 0–655.35 kWh |
| 2-3 | Time to Full (Charging) | `uint16` | minutes | 0–65535 |  |
| 4 | Counter (4&nbsp;bit) | `uint8` | lower 4 bits only | 0–15 | bits 7–4 always 0 |
| 5-6 | reserved |  |  |  |  |
| 7 | CRC8 | `uint8` |  |  |  |

## VCU to BMS

All signals below are unsigned unless otherwise noted.

| Byte | Signal | Type | Value/Range | Notes |
|-----|--------|------|-------------|-------|
| 0 | VehicleOperatingMode | `uint8` | enum |  |
| 1 | RequestBMSShutdown | `uint8` | 0/1 | boolean as `uint8` |
| 2 | RequestContactorClose | `uint8` | 0/1 | boolean as `uint8` |
| 3 | Counter (4&nbsp;bit) | `uint8` | lower 4 bits only | bits 7–4 always 0 |
| 4 | CRC8 | `uint8` |  |  |
| 5-7 | reserved |  |  |  |
