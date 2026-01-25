#include <Arduino.h>
#include <cmath>

#include "bms/coulomb_counting.h"
#include "bms/current.h"
#include "utils/soc_lookup.h"

static CoulombCounting cc;

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float soc_from_ocv(float temp_c, float ocv_v) {
    return SOC_FROM_OCV_TEMP(temp_c, ocv_v) / 100.0f;
}

static float ocv_from_soc(float temp_c, float target_soc) {
    const float soc = clampf(target_soc, 0.0f, 1.0f);
    float best_v = 3.2f;
    float best_err = 1.0e9f;
    for (float v = 3.2f; v <= 4.2f; v += 0.002f) {
        float s = soc_from_ocv(temp_c, v);
        float err = std::fabs(s - soc);
        if (err < best_err) {
            best_err = err;
            best_v = v;
        }
    }
    return best_v;
}
struct CcPersist {
    float cap_est_as = 0.0f;
    float q_total_init = 0.0f;
    bool recal_active = false;
    float recal_start_q = 0.0f;
    float soh = 1.0f;
};

static CcPersist persist;

static void print_cc_state(uint32_t t_s,
                           const char *phase,
                           float v_min,
                           float v_max,
                           float sim_soc,
                           float sim_cap_as,
                           float sim_as_session,
                           float sim_q_total,
                           float sim_v_min,
                           float sim_v_max,
                           float reported_current) {
    Serial.printf(
        "[%lus] %s I=%.1fA as_session=%.1fAs q_total=%.1fAs q_total_init=%.1fAs "
        "soc_cc=%.3f soc_est=%.3f cap_est_as=%.1f q_offset_as=%.1f soh=%.3f "
        "recal_active=%u recal_start_q=%.1f ocv_rest_timer=%.1fs v_min=%.3f v_max=%.3f state=%u "
        "sim_soc=%.3f sim_cap_as=%.1f sim_as=%.1f sim_q_total=%.1f sim_v_min=%.3f sim_v_max=%.3f I_rep=%.1f\n",
        static_cast<unsigned long>(t_s),
        phase,
        param::current,
        param::as_session,
        param::q_total,
        param::q_total_init,
        param::soc_cc,
        param::soc_est,
        param::cap_est_as,
        param::q_offset_as,
        param::soh,
        param::recal_active ? 1U : 0U,
        param::recal_start_q,
        param::ocv_rest_timer,
        v_min,
        v_max,
        static_cast<unsigned int>(param::coulomb_state),
        sim_soc,
        sim_cap_as,
        sim_as_session,
        sim_q_total,
        sim_v_min,
        sim_v_max,
        reported_current);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    param::current = 0.0f;
    param::as = 0.0f;

    const float sim_soc_init = 0.80f;
    const float sim_cap_as_init = CC_CAP_RATED_AS * 0.80f;
    const float sim_q_total_init = sim_soc_init * sim_cap_as_init;

    cc.initialise(CC_CAP_RATED_AS,
                  0.0f,
                  false,
                  0.0f);

    Serial.println("ECC test start");
}

void loop() {
    static uint32_t last_step_ms = 0U;
    static uint32_t sim_time_s = 0U;
    static int phase = 0;
    static bool reset_done = false;

    const uint32_t now = millis();
    if (last_step_ms == 0U) {
        last_step_ms = now;
        return;
    }

    if (now - last_step_ms < 1000U) {
        return;
    }

    last_step_ms += 1000U;
    sim_time_s += 1U;

    // Phase plan: discharge until SOC <= 0.15, rest until OCV settles,
    // then charge, short rest.
    static float sim_soc = 0.80f;
    static float sim_cap_as = CC_CAP_RATED_AS * 0.80f;
    static float sim_as_session = 0.0f;
    static float sim_q_total_init = sim_soc * sim_cap_as;

    const float discharge_a = -1800.0f;
    const float charge_a = 1800.0f;
    const float discharge_drift = 0.00f;

    if (phase == 0 &&
        param::coulomb_state == CoulombCountingState::OPERATING &&
        sim_soc <= 0.15f) {
        phase = 1;
    }

    if (phase == 1 &&
        param::coulomb_state == CoulombCountingState::OPERATING &&
        param::ocv_rest_timer >= 40.0f) {
        phase = 2;
    }

    if (phase == 2 &&
        param::coulomb_state == CoulombCountingState::OPERATING &&
        sim_soc >= 0.90f) {
        phase = 3;
    }

    if (phase == 3 &&
        param::coulomb_state == CoulombCountingState::OPERATING &&
        param::ocv_rest_timer >= 40.0f) {
        phase = 0;
    }

    switch (phase) {
        case 0:
            param::current = discharge_a;
            break;
        case 1:
            param::current = 0.0f;
            break;
        case 2:
            param::current = charge_a;
            break;
        default:
            param::current = 0.0f;
            break;
    }

    // Simulated battery model (80% capacity).
    sim_as_session += param::current * 1.0f;
    const float sim_q_total = sim_q_total_init + sim_as_session;
    sim_soc = clampf(sim_q_total / sim_cap_as, 0.0f, 1.0f);

    // Shunt-integrated As since boot with discharge drift.
    float reported_current = param::current;
    if (param::current < 0.0f) {
        reported_current = param::current * (1.0f + discharge_drift);
    }
    param::as += reported_current * 1.0f;

    const float sim_ocv = ocv_from_soc(25.0f, sim_soc);
    const float v_min = sim_ocv - 0.01f;
    const float v_max = sim_ocv + 0.01f;
    const float sim_v_min = v_min;
    const float sim_v_max = v_max;

    cc.update(v_min, v_max, 25.0f);

    if (phase == 2 && !reset_done && sim_time_s > 900U) {
        // Simulate ECU reset: save persistent data, recreate CC instance, reload.
        persist.cap_est_as = param::cap_est_as;
        persist.q_total_init =
            param::q_total_init + param::as_session + param::q_offset_as;
        persist.recal_active = param::recal_active;
        persist.recal_start_q = param::recal_start_q + param::q_offset_as;
        persist.soh = param::soh;

        cc = CoulombCounting();
        param::as = 0.0f;
        cc.initialise(persist.cap_est_as,
                      persist.q_total_init,
                      persist.recal_active,
                      persist.recal_start_q);
        reset_done = true;
    }

    const char *phase_name =
        (phase == 0) ? "DISCHARGE" :
        (phase == 1) ? "REST" :
        (phase == 2) ? "CHARGE" : "REST";
    print_cc_state(sim_time_s,
                   phase_name,
                   v_min,
                   v_max,
                   sim_soc,
                   sim_cap_as,
                   sim_as_session,
                   sim_q_total,
                   sim_v_min,
                   sim_v_max,
                   reported_current);
}
