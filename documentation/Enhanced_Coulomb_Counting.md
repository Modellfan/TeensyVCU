# Enhanced Coulomb Counting (ECC) - Pseudocode

This document captures the ECC approach exactly as provided.

```text
# Called every 1s (Task1000ms)
# Model: q = b + C * SOC
# SOH = C / C_rated

# --- Parameters / Thresholds ---
float soc_low_set   = 0.20
float soc_high_set  = 0.80
float soc_hyst      = 0.05

float soc_low_reset   = soc_low_set  + soc_hyst   # 0.25
float soc_high_reset  = soc_high_set - soc_hyst   # 0.75

float gamma_b = 0.05     # offset learning gain
float alpha_C = 0.10     # capacity EMA gain

# --- Rated capacity (constant / calibration) ---
float C_rated_as          # rated capacity [As]

# --- Bounds (tune to your system) ---
float C_min = 0.50 * C_rated_as
float C_max = 1.20 * C_rated_as

float b_frac = 0.10                 # |b| <= b_frac * C
float b_step_max_frac = 0.001       # max |Δb| per second as fraction of C

# --- Persistent (NVM) ---
# Values in q-frame are stored rebased against current q_as
float b_as_nvm                # stored offset term [As], rebased
float C_as                    # slope/capacity [As]
float soh                      # SOH = C_as / C_rated_as
bool  have_low_anchor
float q_low_as_nvm            # stored low anchor charge [As], rebased
float soc_low_anchor           # stored as soc_cc (per your request)
bool  was_above_high_set       # persistent cycle-state flag

# --- Runtime (RAM) ---
float q_as                     # integrated charge [As] (from shunt)
float b_as                     # runtime offset: q at SOC=0 [As]
float q_low_as                 # runtime low anchor charge [As]
bool  ocv_valid                # rest & voltage stable
float soc_ocv                  # OCV -> SOC (0..1)
float soc_cc                   # SOC from CC using b_as, C_as

function clamp(x, lo, hi):
    if x < lo: return lo
    if x > hi: return hi
    return x

function LoadPersistent():
    # Load all NVM fields first (b_as_nvm, q_low_as_nvm, C_as, soh, flags...)
    # Convert rebased NVM values back into current q_as frame.
    # If q_as is reset to 0 at boot, runtime values equal NVM values directly.
    b_as     = b_as_nvm     + q_as
    q_low_as = q_low_as_nvm + q_as

function StorePersistent():
    # Rebase q-referenced values to make them invariant to q_as reset on reboot
    b_as_nvm     = b_as     - q_as
    q_low_as_nvm = q_low_as - q_as
    # Store all NVM fields (b_as_nvm, q_low_as_nvm, C_as, soh, flags...)

function Task1000ms():

    # ============================================================
    # RESET LOW ANCHOR when coming from high_set and going below high_reset
    # ============================================================
    if was_above_high_set and (soc_cc <= soc_high_reset):
        have_low_anchor    = false
        was_above_high_set = false

    # ============================================================
    # Track that we reached high region at least once (persistent)
    # ============================================================
    if soc_cc >= soc_high_set:
        was_above_high_set = true

    # ============================================================
    # 1) LOW REGION: b update (state uses soc_cc) + commit anchor at end
    # ============================================================
    if ocv_valid and (soc_cc <= soc_low_set):

        # Residual uses soc_ocv (measurement) against model q = b + C*SOC
        e = q_as - (b_as + C_as * soc_ocv)

        # Raw update
        delta_b = gamma_b * e

        # Step clamp on b update
        b_step_max = b_step_max_frac * C_as
        delta_b = clamp(delta_b, -b_step_max, +b_step_max)

        # Apply update
        b_as = b_as + delta_b

        # Value clamp for b (relative to C)
        b_min = -b_frac * C_as
        b_max = +b_frac * C_as
        b_as  = clamp(b_as, b_min, b_max)

        # Commit low anchor (store soc_cc per your request)
        have_low_anchor = true
        q_low_as        = q_as
        soc_low_anchor  = soc_cc

    # ============================================================
    # 2) HIGH REGION: capacity update + SOH update
    #    - State decision uses soc_cc
    #    - Slope denominator uses soc_ocv (per your correction)
    # ============================================================
    if ocv_valid and have_low_anchor and (soc_cc >= soc_high_set):

        # Two-point slope:
        # numerator uses charge difference; denominator uses current soc_ocv vs stored low anchor soc_cc
        C_new = (q_as - q_low_as) / (soc_ocv - soc_low_anchor)   # intended span ~0.2 -> 0.8

        # Reject gross outliers (optional but recommended)
        C_new = clamp(C_new, 0.30 * C_rated_as, 1.50 * C_rated_as)

        # EMA update
        C_as = (1 - alpha_C) * C_as + alpha_C * C_new

        # Hard clamp
        C_as = clamp(C_as, C_min, C_max)

        # Re-clamp b to new C (keeps consistency after C changes)
        b_min = -b_frac * C_as
        b_max = +b_frac * C_as
        b_as  = clamp(b_as, b_min, b_max)

        # SOH update (allow > 1.0 up to 2.0)
        soh = C_as / C_rated_as
        soh = clamp(soh, 0.0, 2.0)

```
