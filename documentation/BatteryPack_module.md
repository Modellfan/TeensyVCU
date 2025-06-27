# Embedded Automotive Software Module Documentation

## 1. Module Overview

- **Module Name**: `BatteryPack`
- **Purpose**: Represent a collection of BMW i3 battery modules, handle CAN communication with them, manage balancing, and expose pack-level voltage/temperature data.
- **Key Types**: state machine and diagnostic codes defined in `pack.h` lines 20-33.

## 2. Runnable Entities

| Runnable ID   | Description                                      | Execution Cycle |
|---------------|--------------------------------------------------|-----------------|
| `initialize()`| Set up the CAN port and pack variables            | Startup |
| `request_data()`| Poll the next module for measurements            | ~13&nbsp;ms per module |
| `read_message()`| Receive CAN frames and update module states      | 2&nbsp;ms timer |
| `print()`     | Debug printout of pack and module status          | Debug/diagnostic |

Runnables are declared in `pack.h` lines 39‑44.

## 3. RTE (Run-Time Environment) Signals

### Input Signals

| Signal Name | Description |
|-------------|-------------|
| `read_message()` | Receives CAN frames from battery modules and updates their data |
| `set_balancing_active(bool)` | Enable or disable cell balancing |
| `set_balancing_voltage(float)` | Set the target voltage for balancing |

### Output Signals

| Signal Name | Description |
|-------------|-------------|
| `request_data()` | Sends CAN polling frames to query module measurements |
| `send_message(CANMessage *)` | Low level helper used to transmit a CAN frame |
| `get_balancing_active()` | Indicates whether balancing is active |
| `get_balancing_voltage()` | Returns the configured balancing target voltage |
| `get_any_module_balancing()` | Checks if any module is currently balancing |
| `get_lowest_cell_voltage()` | Lowest cell voltage across all modules |
| `get_highest_cell_voltage()` | Highest cell voltage across all modules |
| `get_pack_voltage()` | Current total pack voltage |
| `get_delta_cell_voltage()` | Difference between highest and lowest cell voltage |
| `get_cell_voltage(byte, float &)` | Retrieves the voltage for a specific cell |
| `get_lowest_temperature()` | Lowest module temperature |
| `get_highest_temperature()` | Highest module temperature |
| `get_cell_temperature(byte, float &)` | Retrieves the temperature for a specific cell |
| `getState()` | Current pack state enumeration |
| `getDTC()` | Current diagnostic trouble code |
| `print()` | Debug printout of pack and module status |

## 4. Configuration Parameters

| Parameter Name              | Default Value | Description |
|-----------------------------|---------------|-------------|
| `MODULES_PER_PACK`          | `8` | Number of modules per pack |
| `BATTERY_CAN`               | `can2` | CAN interface for the pack |
| `PACK_WAIT_FOR_NUM_MODULES` | `8` | Modules required before `OPERATING` state |
| `PACK_ALIVE_TIMEOUT`        | `300` ms | Module alive timeout |

Parameters originate from `settings.h` lines 10‑26.

## 5. Hardware Interface

| Interface Name | Type | Signal Type | Description |
|----------------|------|-------------|-------------|
| `modules[MODULES_PER_PACK]` | Internal array | objects | Represents the physical CMUs |
| `BATTERY_CAN` | CAN bus | Input/Output | Communicates with modules |

## 6. Error Handling

- Diagnostic trouble codes (`DTC_PACK_*`) track CAN send/initialization errors and module faults.
- State machine transitions from `INIT` to `OPERATING` when enough modules are detected and to `FAULT` on errors.
- Failed CAN transmissions set `DTC_PACK_CAN_SEND_ERROR`.

## 7. Open Points

From the functional specification (lines 92‑101):

- Fetch SoC from shunt and store in memory
- Broadcast BMS mode
- DRIVE_INHIBIT output
- Use other core for comms?
- Implement balancing
- Implement watchdog
- Detect startup state automatically

## Mermaid Overview

```
:::mermaid
flowchart TB
    subgraph BatteryPack
        direction TB
        subgraph "Input Signals"
            readmsg["read_message() - CAN frames from modules"]
            setbal["set_balancing_active(bool)"]
            setvoltage["set_balancing_voltage(float)"]
        end
        subgraph "Output Signals"
            reqdata["request_data()/send_message()"]
            getters["Getter methods\n(voltages, temps, diagnostics)"]
        end
        subgraph "Connected Hardware"
            modules["BatteryModule[MODULES_PER_PACK]"]
            canbus["BATTERY_CAN (CAN bus)"]
        end
    end
```
