# TeensyVCU Code Reference

This document provides an overview of the TeensyVCU firmware modules and the
responsibilities of the primary classes, functions, and utilities that make up
the codebase.  The goal is to give developers a single place to understand how
components fit together and what each callable element is responsible for.

## Top-Level Runtime Flow (`src/main.cpp`)

| Symbol | Description |
| --- | --- |
| `setup()` | Configures serial interfaces, prints build metadata, starts the cooperative scheduler, and enables all periodic tasks that drive the shunt, contactors, battery pack polling, and higher-level BMS routines. |
| `loop()` | Runs the cooperative task scheduler and services interactive serial console commands. |
| `wdtCallback()` | Watchdog handler that emits a diagnostic notice shortly before a watchdog-triggered reset. |

Global singletons created here provide cross-module access to hardware
abstractions: `Scheduler scheduler`, `Shunt_ISA_iPace shunt`,
`Contactormanager contactor_manager`, `BatteryPack batteryPack`, and
`BMS battery_manager`.

## Task Wiring and Runtime Services (`src/comms_bms.*`)

These helpers expose `enable_*` functions that register periodic tasks with the
`TaskScheduler` instance.

| Function | Purpose |
| --- | --- |
| `enable_update_shunt()` / `update_shunt()` | Initializes the ISA shunt current sensor and repeatedly calls `Shunt_ISA_iPace::update()` every 5 ms. |
| `enable_update_contactors()` / `update_contactors()` | Sets up the contactor manager and services its state machine on the `CONTACTOR_TIMELOOP` interval. |
| `enable_handle_battery_CAN_messages()` / `handle_battery_CAN_messages()` | Initializes the BMW i3 battery CAN interface and forwards incoming frames to the pack model. |
| `enable_poll_battery_for_data()` / `poll_battery_for_data()` | Issues polling commands to the BMW i3 modules approximately every 13 ms. |
| `enable_BMS_tasks()` | Registers the `BMS::Task2Ms`, `Task10Ms`, `Task100Ms`, and `Task1000Ms` periodic jobs that implement the BMS runtime. |
| `enable_BMS_monitor()` | Initializes the high-level BMS component and starts placeholder monitor tasks (currently stubs). |
| `enable_led_blink()` / `led_blink()` | Toggles the onboard LED every second to indicate liveness. |
| `enable_update_system_load()` / `update_system_load()` | Skeleton for tracking CPU load (currently a stub). |
| `enable_print_debug()` / `print_debug()` | Schedules a hook for additional periodic diagnostics (currently empty). |

## Interactive Console (`src/serial_console.*`, `src/console_printer.*`)

* `ConsolePrinter` encapsulates formatted writes to `Serial`.
* `serial_console()` parses commands from the USB serial interface and exposes
  controls to inspect and manipulate pack, module, contactor, shunt, and BMS
  state. Supporting helpers (`print_pack_status()`, `print_module_status()`,
  `print_contactor_status()`, etc.) translate enumerations and DTC bitfields
  into human-readable descriptions.
* `enable_serial_console()` is declared but not yet implemented; console
  functionality currently runs from `loop()`.

## Hardware Abstraction Layers

### ISA Shunt (`src/bms/current.*`)

| Method | Description |
| --- | --- |
| `initialise()` | Configures the CAN interface used by the ISA shunt and enters the `INIT` state. Records CAN initialization errors as DTCs. |
| `update()` | Processes raw shunt CAN frames, detects timeouts and over-current/over-temperature conditions, and keeps track of current, temperature, coulomb counting, and current derivative metrics. |
| `calculate_current_values()` | Internal helper that updates moving averages, integrates ampere-seconds, and computes current derivatives. |
| `getCurrent()` / `getTemperature()` / `getAmpereSeconds()` / `getCurrentAverage()` / `getCurrentDerivative()` | Expose the most recent sensor metrics to other modules. `getAmpereSeconds()` also resets the integration accumulator. |
| `getState()` / `getDTC()` | Report the shunt state machine state and aggregated diagnostic flags. |

### Contactor Control (`src/bms/contactor.*`, `src/bms/contactor_manager.*`)

**`Contactor`** encapsulates a single contactor driver with optional feedback
monitoring.

| Method | Description |
| --- | --- |
| `Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms, bool allowExternalControl)` | Constructor wiring the control and feedback pins, debounce, timeout, and whether unexpected state changes are tolerated. |
| `initialise()` | Sets the initial state and validates the feedback pin. If `CONTACTOR_DISABLE_FEEDBACK` is defined, skips feedback checks and starts open for bench testing. |
| `close()` / `open()` | Command the contactor to change state. When feedback is enabled these trigger state-machine transitions and enforce timing constraints; in debug mode they immediately change outputs. |
| `update()` | Runs the state machine that waits for feedback transitions, applies debounce logic, and records timeout or unexpected-state DTCs. |
| `getState()` / `getInputPin()` / `getOutputPin()` / `getDTC()` / `getDTCString()` | Observability helpers for the current contactor status. |

**`Contactormanager`** orchestrates the precharge and positive contactors using
`Contactor` instances and monitors auxiliary inputs.

| Method | Description |
| --- | --- |
| `initialise()` | Validates auxiliary inputs (negative contactor and power supply), initializes child contactors, and enters `OPEN` unless faults are detected. |
| `close()` / `open()` | Set the desired state target (`CLOSED` or `OPEN`), which the periodic `update()` routine works toward. |
| `update()` | Drives the precharge-first, positive-second closing sequence, enforces opening order, monitors for hardware faults, and reflects faults in the manager DTC. Also maintains copies of auxiliary input states. |
| `getState()` / `getDTC()` | Return the aggregate manager state and diagnostic code. |
| `getPositiveState()` / `getPositiveDTC()` / `getPositiveInputPin()` and analogous `getPrecharge*` accessors | Expose the underlying contactor status for diagnostics and console output. |
| `isNegativeContactorClosed()` / `isContactorVoltageAvailable()` | Surface auxiliary input statuses. |

### BMW i3 Battery Pack Model (`src/bms/battery i3/*.cpp`)

**`BatteryModule`** represents a single 12-cell module.

* `process_message()` parses BMW i3 CMU CAN frames, updates cell voltages,
  temperatures, balancing flags, tracks CRC failures, and manages the module
  state machine (`INIT` → `OPERATING` → `FAULT`).
* `check_alive()` tracks watchdog timeouts based on `PACK_ALIVE_TIMEOUT` and
  sets the `TIMED_OUT` DTC when communication stalls.
* Accessors return per-module metrics such as module voltage, lowest/highest
  cell values, internal temperature, balancing status, and DTC strings.
* Internal helpers `check_if_module_data_is_populated()`, `plausibilityCheck()`,
  and `check_crc()` ensure data integrity before declaring the module
  operational.

**`BatteryPack`** aggregates multiple modules and owns the BMW i3 pack CAN bus.

* `initialize()` configures the CAN controller and resets polling state.
* `request_data()` implements the BMW polling sequence, including balancing
  commands and frame counters.
* `read_message()` dispatches incoming CAN frames to modules, checks for
  timeouts, and advances the pack state machine. It sets pack-level DTCs when
  modules fault or CAN sends fail.
* Accessors provide pack-wide derived quantities such as pack voltage,
  highest/lowest cell voltage, cell temperature extremes, balance controls, and
  delta metrics used elsewhere in the firmware.

### High-Level Battery Management (`src/bms/battery_manager.*`)

The `BMS` class bridges the pack, shunt, contactor manager, and vehicle control
unit (VCU).

Key responsibilities:

* **Initialization & Persistence**: `initialize()` configures the BMS CAN bus and
  seeds runtime state from `PersistentDataStorage`.
* **Task Hooks**: `Task2Ms()`, `Task10Ms()`, `Task100Ms()`, and `Task1000Ms()` are
  scheduled by `enable_BMS_tasks()` and dispatch the core logic (CAN polling,
  state machine, SOC/SoE calculations, current-limit lookups, balancing logic,
  etc.).
* **State Machine**: `update_state_machine()` ensures the BMS transitions to
  `FAULT` whenever the pack, contactor manager, or shunt report faults, and to
  `OPERATING` once all subsystems are ready.
* **Energy & SOC Estimation**: `update_soc_ocv_lut()`,
  `update_soc_coulomb_counting()`, `correct_soc()`, and
  `update_energy_metrics()` compute SOC, average power, remaining Wh, and time
  estimates. Coulomb-counting and some advanced functions are placeholders for
  future refinement.
* **Current Limits**: `lookup_current_limits()` and
  `lookup_internal_resistance_table()` use LUT helpers to determine permissible
  charge/discharge limits. `calculate_voltage_derate()` applies additional
  derating based on the worst cell voltages.
* **Balancing Control**: `update_balancing()` decides when to enable passive
  balancing based on vehicle state, temperature limits, voltage deltas, and
  ongoing balancing activity. It maintains the `balancing_finished` flag used in
  status reporting.
* **CAN Messaging**: `read_message()` ingests VCU commands (including contactor
  control) and flags timeouts. `send_battery_status_message()` builds and emits
  the suite of BMS status frames (voltage, temperature, limits, SOC, HMI data),
  using `can_crc8()` for checksums.
* **Persistence Utilities**: `apply_persistent_data()` and
  `collect_persistent_data()` bridge the runtime data and EEPROM storage.

### Persistent Data Storage (`src/persistent_data_storage.h`)

`PersistentDataStorage` provides a simple EEPROM-backed circular log of measured
energy capacity information.

* `begin()` scans the available slots for the most recent valid record and caches
  it.
* `load()` returns the cached `PersistentData` (initializing the storage on the
  fly if needed).
* `save()` writes a new record to the next slot, rotating through `kSlotCount`
  entries and updating cached metadata.
* Internally `scan_slots()`, `read_slot()`, and `write_slot()` handle probing and
  updating EEPROM slots with per-record CRC protection.

## Utility Headers (`src/utils/*`)

| File | Highlights |
| --- | --- |
| `Map2D3D.h`, `interpolate.h` | Generic 2D/3D lookup table helpers with linear/bilinear interpolation, used by the LUT-driven estimation routines. |
| `current_limit_lookup.h` | Temperature-indexed lookup tables for peak and continuous charge/discharge current limits, exposed through macros such as `DISCHARGE_PEAK_CURRENT_LIMIT(t)`. |
| `resistance_lookup.h` | SOC- and temperature-based internal resistance lookup tables for two current scenarios. |
| `soc_lookup.h` | SOC estimation LUT based on open-circuit voltage and temperature. |
| `can_crc.h` | 32-bit CRC routines and the BMW-specific `can_crc8()` helper. |
| `can_packer.h/.cpp` | Bit-level helpers for packing/unpacking CAN payload fields in either endianness. |

## Diagnostics and Console Commands (`src/serial_console.cpp`)

Console commands (entered over USB serial) provide runtime observability:

| Command | Action |
| --- | --- |
| `c` / `o` | Close or open the main contactors. |
| `s` | Print aggregated contactor manager status and DTCs. |
| `p` | Display pack-level metrics (voltage, temperatures, balancing state). |
| `b` | Toggle pack balancing. |
| `vX.XX` | Set the balancing voltage target. |
| `mX` | Show detailed status for module `X`. |
| `B` | Print high-level BMS state, SOC, current limits, and vehicle status. |
| `i` | Show shunt current, temperature, ampere-seconds, and diagnostic info. |
| `F` | Print firmware build timestamp. |
| `h` / `?` | Show the command help text. |

Helper routines convert internal enumerations and diagnostic bitmasks to human
readable strings for the console output.

## Open TODOs

* `src/bms/battery_manager.cpp`: Replace the SOC proxy with a real state of
  energy (SoE) computation once available (`update_energy_metrics()` comment in
  the energy metrics section).

