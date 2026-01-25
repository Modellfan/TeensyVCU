# Enhanced Coulomb Counting (ECC) - Pseudocode

This document captures the ECC approach exactly as provided, with one allowed
deviation: `as_session` is sourced from the shunt (integrated ampere-seconds)
instead of integrating `I * dt` in software.

```text
# --- Persistent Variables (Stored in NVM) ---
float cap_est_as           # Estimated usable capacity [As]
float q_total_init         # Total integrated charge at last shutdown [As]
bool recal_active          # Is a recalibration in progress?
float recal_start_q        # Charge at start of recalibration (includes offset) [As]
float soh                  # Estimated SOH = cap_est_as / cap_rated_as

# --- Runtime Variables (Reset on boot) ---
float as_session           # Measured charge from shunt since boot [As]
float ocv_rest_timer = 0   # Time in rest condition [s]
float q_offset_as = 0      # Runtime-only OCV correction offset [As]
float soc_cc               # SOC from coulomb counting
float soc_est              # Final corrected SOC
float q_total              # Total integrated charge [As]

# --- Constants ---
float K = 0.1                    # OCV correction gain
float rest_threshold = 10.0      # Idle current threshold [A]
float rest_time_min = 15.0       # Required rest time for OCV reading [s]
float soc_low_threshold = 0.20
float soc_high_threshold = 0.80
float alpha = 0.5                # Smoothing factor for capacity update
float cap_rated_as               # Nominal battery capacity [As]

# --- On Boot ---
function boot():
    q_offset_as = 0              # Drift resets at boot

# --- On Shutdown ---
function shutdown():
    q_total = q_total_init + as_session + q_offset_as
    recal_start_q = recal_start_q + q_offset_as
    # Save q_total_init, cap_est_as, recal_start_q, recal_active, soh

# --- SOC Update Function ---
function update_soc(float I, float V_min, float V_max, float dt):

    # --- Coulomb Counting ---
    # as_session updated externally by shunt hardware.
    q_total = q_total_init + as_session
    soc_cc = clamp((q_total + q_offset_as) / cap_est_as, 0.0, 1.0)
    soc_est = soc_cc

    # --- Rest Detection ---
    if abs(I) < rest_threshold:
        ocv_rest_timer += dt
    else:
        ocv_rest_timer = 0

    # --- OCV-Based SOC Correction ---
    if ocv_rest_timer > rest_time_min:
        soc_hi = OCV_to_SOC(V_max)
        soc_lo = OCV_to_SOC(V_min)

        if soc_hi > soc_high_threshold:
            soc_ocv = soc_hi
        elif soc_lo < soc_low_threshold:
            soc_ocv = soc_lo
        else:
            soc_ocv = None

        if soc_ocv is not None:
            soc_est += K * (soc_ocv - soc_est)
            q_offset_as += K * (soc_ocv - soc_cc) * cap_est_as

    # --- Start Recalibration at Low SOC ---
    if soc_cc < soc_low_threshold and not recal_active:
        recal_active = true
        recal_start_q = q_total

    # --- Complete Recalibration at High SOC ---
    if soc_cc > soc_high_threshold and recal_active:
        delta_soc = soc_cc - soc_low_threshold
        delta_q = q_total - recal_start_q

        new_cap = delta_q / delta_soc
        cap_est_as = alpha * new_cap + (1 - alpha) * cap_est_as
        soh = cap_est_as / cap_rated_as

        recal_active = false
```
