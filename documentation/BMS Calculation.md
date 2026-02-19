:::mermaid
flowchart TD

 

  %% Inputs

  A1["Sensor Inputs:<br>cell_voltage[], cell_temp[], pack_current, dt, invalid_data"]

 

  %% SOC Calculation

  A3["Coulomb Counting As<br>Current Integration with recalibration<br>fn: calculate_soc_coulomb_counting()"]

 

  %% Table Lookups

  B1["Current Limits Lookup<br>from Temperature<br>fn: lookup_current_limits()"]

  B2["IR Table Lookup<br>from Temperature and SOC<br>fn: lookup_internal_resistance_table()"]

 

  %% IR Estimation

  C1["IR Online Estimation<br>from cell_voltage[], pack_current<br>fn: estimate_internal_resistance_online()"]

  C2["Select Maximum IR<br>from Table or Estimation<br>fn: select_internal_resistance_used()"]

 

  %% Voltage Derating

  D1["Piecewise Linear<br>Low Voltage Derating<br>fn: calculate_voltage_derate()"]

  D2["Piecewise Linear<br>High Temperature Derating<br>fn: calculate_temperature_derate()"]

  D3["Current Limit Watchdog<br>fn: monitor_current_limit()"]

 

  %% RMS Calculation

  E1["RMS / EMA Calculation<br>with Soft Clamp<br>fn: calculate_rms_ema()"]

 

  %% Dynamic Voltage Limit

  F1["Dynamic Voltage Limit<br>fn: calculate_dynamic_voltage_limit()"]

  F2["On Discharge Path Detract Other Consumers Current<br>fn: calculate_other_consumers()"]

 

  %% Final Limit

  G1{"Select Minimum:<br>limit_rms_ema, limit_voltage_dynamic<br>fn: select_current_limit()"}

 

  %% Limp Home

  LH["User Defined Parameter"]

  LH2{"Invalid Input Parameter?<br>If Yes: 0<br>If No: output = input <br>fn: calculate_invalid()"}

  LHSel{"Select Maximum:<br> So we never Fall back below Limp Home Current."}

 

  %% Rate Limiter

  RL["Rate Limiter<br>Smooth output changes<br>fn: rate_limit_current()"]

 

  %% Output

  H1["Final Parameter Output"]

 

  %% Connections (with your class variable names)

  A1 -- "ampere_seconds<br>cell_voltage<br>avg_cell_temp" --> A3

  A3 -- "soc_coulomb_counting" --> B2

  A3 -- "measured_capacity_Ah<br>soh_coulomb_counting<br>soc_coulomb_counting<br>soc_ocv" --> H1

  A1 -- "low_cell_temp" --> B1

  A1 -- "cell_temp" --> B2

  C1 -- "internal_resistance_estimated" --> C2

  B2 -- "internal_resistance_table" --> C2

  C2 -- "internal_resistance_used" --> F1

  A1 -- "cell_voltage" --> F1

  A1 -- "low_cell_voltage" --> D1

  A1 -- "high_cell_temp" --> D2

  B1 -- "chg_limit_rms<br>dis_limit_rms" --> E1

  B1 -- "chg_limit_peak<br>dis_limit_peak" --> E1

  E1 -- "chg_limit_ema<br>dis_limit_ema" --> G1

  F1 -- "current_limit_voltage_dynamic" --> G1

  A1 -- "pack_current, dt" --> E1

  G1 -- "current_limit_stage_1" --> D1

  D1 -- "current_limit_stage_2" --> D2

  D2 -- "current_limit_stage_3" --> F2

    F2 -- "current_limit_stage_4" --> LH2

    LH2 -- "current_limit_stage_5" --> LHSel

  A1 -- "cell_voltage, pack_current" --> C1

  A1 -- "invalid_data" --> LH2

  LH -- "current_limit_limp_home" --> LHSel

  LHSel -- "current_limit_selected" --> RL

  RL -- "current_limit_final" --> H1

  RL -- "current_limit_final" --> D3

  D3 -- "current_limit_hit" --> H1

  A1 -- "pack_current" --> D3
