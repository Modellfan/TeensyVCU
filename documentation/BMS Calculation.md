:::mermaid
flowchart TD

  %% Inputs
  A1["Sensor Inputs:<br>cell_voltage[], cell_temp[], pack_current, dt, invalid_data"]

  %% SOC Calculation
  A2["OCV-Temp LUT<br>Initial SOC Estimate<br>fn: calculate_soc_ocv_lut()"]
  A3["Coulomb Counting<br>Current Integration with recalibration<br>fn: calculate_soc_coulomb_counting()"]
  A4["SOC Correction<br>Fuse LUT & Coulomb values<br>fn: calculate_soc_correction()"]

  %% SOH Block
  SOH["SOH Estimation (monitoring only)<br>measured_capacity_Ah = integrated charge over full cycle<br>fn: calculate_soh()"]

  %% Table Lookups
  B1["Current Limits Lookup<br>from Temperature<br>fn: lookup_current_limits()"]
  B2["IR Table Lookup<br>from Temperature and SOC<br>fn: lookup_internal_resistance_table()"]

  %% IR Estimation
  C1["IR Online Estimation<br>from cell_voltage[], pack_current<br>fn: estimate_internal_resistance_online()"]
  C2["Select Maximum IR<br>from Table or Estimation<br>fn: select_internal_resistance_used()"]

  %% Voltage Derating
  D1["Piecewise Linear<br>Voltage Derating<br>fn: calculate_voltage_derate()"]

  %% RMS Calculation
  E1["RMS / EMA Calculation<br>with Soft Clamp<br>fn: calculate_rms_ema()"]

  %% Dynamic Voltage Limit
  F1["Dynamic Voltage Limit<br>fn: calculate_dynamic_voltage_limit()"]

  %% Final Limit
  G1{"Select Minimum:<br>current_limit_peak, current_limit_rms_derated, current_limit_voltage_dynamic, current_limit_rms<br>fn: select_current_limit()"}

  %% Limp Home
  LH["Limp Home Limit<br>if invalid_data<br>fn: calculate_limp_home_limit()"]
  LHSel{"Limp Home?<br>If Yes: Output current_limit_limp_home<br>If No: Output current_limit_allowed<br>fn: select_limp_home()"}

  %% Rate Limiter
  RL["Rate Limiter<br>Smooth output changes<br>fn: rate_limit_current()"]

  %% Output
  H1["Allowed Current Command<br>current_limit_final"]

  %% Connections (with your class variable names)
  A1 -- "cell_voltage, cell_temp" --> A2
  A1 -- "pack_current, dt, cell_voltage" --> A3
  A2 -- "soc_ocv_lut" --> A4
  A3 -- "soc_coulomb_counting" --> A4
  A2 -- "soc_ocv_lut (for recalibration)" --> A3
  A4 -- "soc" --> B2
  A3 -- "measured_capacity_Ah" --> SOH
  A1 -- "cell_temp" --> B1
  A1 -- "cell_temp" --> B2
  C1 -- "internal_resistance_estimated" --> C2
  B2 -- "internal_resistance_table" --> C2
  C2 -- "internal_resistance_used" --> F1
  A1 -- "cell_voltage" --> F1
  A1 -- "cell_voltage" --> D1
  B1 -- "current_limit_rms" --> D1
  B1 -- "current_limit_peak" --> G1
  E1 -- "current_limit_rms" --> G1
  F1 -- "current_limit_voltage_dynamic" --> G1
  A1 -- "pack_current, dt" --> E1
  G1 -- "current_limit_allowed" --> LHSel
  D1 -- "current_limit_rms_derated" --> E1
  A1 -- "cell_voltage, pack_current" --> C1

  %% SOH Path (monitor/logging only)
  %% Already: A3 -- "measured_capacity_Ah" --> SOH

  %% Limp Home Path
  A1 -- "invalid_data" --> LHSel
  LH -- "current_limit_limp_home" --> LHSel
  LHSel -- "current_limit_selected" --> RL
  RL -- "current_limit_final" --> H1
