# Coulomb Counting Recalibration Flowchart

:::mermaid
flowchart TD

  %% Input Block
  A1["Current Sensor:<br>getAmpereSeconds()"]
  A2["Cell Voltages:<br>cell_voltage[]"]
  A3["Cell Temps:<br>cell_temp[]"]

  %% State Variables
  S1["ampere_seconds_since_start"]
  S2["ampere_seconds_initial<br>(from EEPROM)"]
  S3["measured_capacity_Ah<br>(from EEPROM)"]

  %% Main Counting Loop
  C1["Update:<br>ampere_seconds_since_start += getAmpereSeconds()"]
  C2["Calculate current_charge_Ah:<br>(ampere_seconds_initial + ampere_seconds_since_start) / 3600"]
  C3["Calculate soc_coulomb_counting:<br>current_charge_Ah / measured_capacity_Ah"]

  %% Recalibration Triggers
  R1{"Recalibration Trigger?<br>(e.g., SOC ≈ 20% or 0%,<br>cell_voltage threshold,<br>OCV rest, etc.)"}

  %% Recalibration Logic
  R2["If recalibration at SOC_ref (e.g. 20%):<br>
      soc_coulomb_counting = SOC_ref<br>
      ampere_seconds_initial = SOC_ref * measured_capacity_Ah * 3600 - ampere_seconds_since_start<br>
      Save ampere_seconds_initial to EEPROM"]
  R3["If full cycle (0%–100% or 100%–0%) detected:<br>
      measured_capacity_Ah = |ampere_seconds_full - ampere_seconds_empty| / 3600<br>
      Save measured_capacity_Ah to EEPROM"]

  %% Output / Diagnostics
  O1["SOC Output: soc_coulomb_counting"]
  O2["SOH Output: measured_capacity_Ah"]

  %% Connections
  A1 -- "ampere_seconds_delta" --> C1
  C1 -- "ampere_seconds_since_start" --> C2
  S2 -- "ampere_seconds_initial" --> C2
  C2 -- "current_charge_Ah" --> C3
  S3 -- "measured_capacity_Ah" --> C3
  C3 -- "soc_coulomb_counting" --> O1

  %% Recalibration triggers
  A2 -- "cell_voltage[]" --> R1
  A3 -- "cell_temp[]" --> R1
  C3 -- "soc_coulomb_counting" --> R1

  R1 -- "Yes: at SOC_ref (e.g. 20%)" --> R2
  R2 -- "ampere_seconds_initial updated" --> S2
  R1 -- "Yes: full cycle (0% or 100%)" --> R3
  R3 -- "measured_capacity_Ah updated" --> S3

  S3 -- "measured_capacity_Ah" --> O2

  %% EEPROM Storage
  S2 -- "ampere_seconds_initial (persistent)" --> EEPROM1["EEPROM/Flash"]
  S3 -- "measured_capacity_Ah (persistent)" --> EEPROM1

  %% Reset ampere_seconds_since_start at recal
  R2 -- "reset?" --> C1
  R3 -- "reset?" --> C1
:::
