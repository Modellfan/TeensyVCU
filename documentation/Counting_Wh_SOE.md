:::mermaid
flowchart TD

  %% Input Block
  A1["Power Integration:<br>getWattSeconds() ≈ V_pack * I_pack * dt"]
  A2["Cell Voltages:<br>cell_voltage[]"]
  A3["Cell Temps:<br>cell_temp[]"]

  %% State Variables
  S1["watt_seconds_since_start"]
  S2["energy_initial_Wh<br>(from EEPROM)"]
  S3["measured_capacity_Wh<br>(from EEPROM)"]

  %% Main Energy-Counting Loop
  C1["Update:<br>watt_seconds_since_start += getWattSeconds()"]
  C2["Calculate current_energy_Wh:<br>(energy_initial_Wh + watt_seconds_since_start)"]
  C3["Calculate soe_energy_counting:<br>current_energy_Wh / measured_capacity_Wh"]

  %% Recalibration Triggers
  R1{"Recalibration Trigger?<br>(e.g., SOE ≈ 20% or 0%,<br>cell_voltage threshold,<br>OCV rest, etc.)"}

  %% Recalibration Logic
  R2["If recalibration at SOE_ref (e.g. 20%):<br>
      soe_energy_counting = SOE_ref<br>
      energy_initial_Wh = SOE_ref * measured_capacity_Wh * 3600 - watt_seconds_since_start<br>
      Save energy_initial_Wh to EEPROM"]
  R3["If full usable energy cycle (0%–100% or 100%–0%) detected:<br>
      measured_capacity_Wh = |watt_seconds_full - watt_seconds_empty| / 3600<br>
      Save measured_capacity_Wh to EEPROM"]

  %% Output / Diagnostics
  O1["SOE Output: soe_energy_counting"]
  O2["SOH-Energy Output: measured_capacity_Wh"]

  %% Connections
  A1 -- "watt_seconds_delta" --> C1
  C1 -- "watt_seconds_since_start" --> C2
  S2 -- "energy_initial_Wh" --> C2
  C2 -- "current_energy_Wh" --> C3
  S3 -- "measured_capacity_Wh" --> C3
  C3 -- "soe_energy_counting" --> O1

  %% Recalibration triggers
  A2 -- "cell_voltage[]" --> R1
  A3 -- "cell_temp[]" --> R1
  C3 -- "soe_energy_counting" --> R1

  R1 -- "Yes: at SOE_ref (e.g. 20%)" --> R2
  R2 -- "energy_initial_Wh updated" --> S2
  R1 -- "Yes: full cycle (0% or 100%)" --> R3
  R3 -- "measured_capacity_Wh updated" --> S3

  S3 -- "measured_capacity_Wh" --> O2

  %% EEPROM Storage
  S2 -- "energy_initial_Wh (persistent)" --> EEPROM1["EEPROM/Flash"]
  S3 -- "measured_capacity_Wh (persistent)" --> EEPROM1

  %% Reset watt_seconds_since_start at recal
  R2 -- "reset?" --> C1
  R3 -- "reset?" --> C1
