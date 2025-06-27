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

- CAN frames from battery modules handled in `read_message()`.
- Balancing commands via `set_balancing_active(bool)` and `set_balancing_voltage(float)`.

### Output Signals

- Polling frames sent with `request_data()`/`send_message()`.
- Getter methods for voltages, temperatures, balancing status, state, and diagnostic codes.

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
