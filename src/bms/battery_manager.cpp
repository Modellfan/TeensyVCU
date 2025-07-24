#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery_manager.h"
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "utils/can_packer.h"
#include "utils/can_crc.h"
#include "utils/current_limit_lookup.h"
#include "utils/soc_lookup.h"
#include <math.h>

// #define DEBUG

BMS::BMS(BatteryPack &_batteryPack, Shunt_ISA_iPace &_shunt, Contactormanager &_contactorManager) : batteryPack(_batteryPack), shunt(_shunt), contactorManager(_contactorManager)
{
    state = INIT;
    dtc = DTC_BMS_NONE;
    moduleToBeMonitored = 0;
    max_charge_current = 0.0f;
    max_discharge_current = 0.0f;
    current_limit_peak_discharge = 0.0f;
    current_limit_rms_discharge = 0.0f;
    current_limit_peak_charge = 0.0f;
    current_limit_rms_charge = 0.0f;
    current_limit_rms_derated_discharge = 0.0f;
    current_limit_rms_derated_charge = 0.0f;
    ready_to_shutdown = false;
    vehicle_state = STATE_SLEEP;
    msg1_counter = 0;
    msg2_counter = 0;
    msg3_counter = 0;
    msg4_counter = 0;
    msg5_counter = 0;
    vcu_counter = 0;
    last_vcu_msg = 0;
    vcu_timeout = false;
    balancing_finished = false;
}

void BMS::initialize()
{
    // Set up CAN port
    ACAN_T4_Settings settings(500 * 1000); // 500 kbit/s
    settings.mTransmitBufferSize = 800;
    const uint32_t errorCode = ACAN_T4::BMS_CAN.begin(settings);

#ifdef DEBUG
    Serial.print("Bitrate prescaler: ");
    Serial.println(settings.mBitRatePrescaler);
    Serial.print("Propagation Segment: ");
    Serial.println(settings.mPropagationSegment);
    Serial.print("Phase segment 1: ");
    Serial.println(settings.mPhaseSegment1);
    Serial.print("Phase segment 2: ");
    Serial.println(settings.mPhaseSegment2);
    Serial.print("RJW: ");
    Serial.println(settings.mRJW);
    Serial.print("Triple Sampling: ");
    Serial.println(settings.mTripleSampling ? "yes" : "no");
    Serial.print("Actual bitrate: ");
    Serial.print(settings.actualBitRate());
    Serial.println(" bit/s");
    Serial.print("Exact bitrate ? ");
    Serial.println(settings.exactBitRate() ? "yes" : "no");
    Serial.print("Distance from wished bitrate: ");
    Serial.print(settings.ppmFromWishedBitRate());
    Serial.println(" ppm");
    Serial.print("Sample point: ");
    Serial.print(settings.samplePointFromBitStart());
    Serial.println("%");
#endif

    if (0 == errorCode)
    {
        Serial.println("BMS CAN ok");
    }
    else
    {
        Serial.print("Error BMS CAN: 0x");
        Serial.println(errorCode, HEX);
        dtc = static_cast<DTC_BMS>(dtc | DTC_BMS_CAN_INIT_ERROR);
    }

    contactorManager.close();
}

//###############################################################################################################################################################################
//  Runnables
//###############################################################################################################################################################################

void BMS::Task2Ms() { read_message(); }

void BMS::Task10Ms()
{
    // Reserved for future use
}

void BMS::Task100Ms()
{
    update_state_machine();
    send_battery_status_message();
}

void BMS::Task1000Ms()
{
    update_soc_ocv_lut();
    update_soc_coulomb_counting();
    correct_soc();
}

//###############################################################################################################################################################################
//  BMS State Machine
//###############################################################################################################################################################################

void BMS::update_state_machine()
{
    BatteryPack::STATE_PACK pack_state = batteryPack.getState();
    Contactormanager::State contactor_state = contactorManager.getState();
    Shunt_ISA_iPace::STATE_ISA shunt_state = shunt.getState();

    // Transition to fault state whenever a critical component reports a fault
    if (pack_state == BatteryPack::FAULT ||
        contactor_state == Contactormanager::FAULT ||
        shunt_state == Shunt_ISA_iPace::FAULT)
    {
        if (pack_state == BatteryPack::FAULT)
        {
            dtc = static_cast<DTC_BMS>(dtc | DTC_BMS_PACK_FAULT);
        }
        if (contactor_state == Contactormanager::FAULT)
        {
            dtc = static_cast<DTC_BMS>(dtc | DTC_BMS_CONTACTOR_FAULT);
        }
        if (shunt_state == Shunt_ISA_iPace::FAULT)
        {
            dtc = static_cast<DTC_BMS>(dtc | DTC_BMS_SHUNT_FAULT);
        }
        state = FAULT;
        max_discharge_current = BMS_LIMP_HOME_DISCHARGE_CURRENT;
        max_charge_current = BMS_LIMP_HOME_CHARGE_CURRENT;
        return;
    }

    switch (state)
    {
    case INIT:
        if (pack_state == BatteryPack::OPERATING &&
            shunt_state == Shunt_ISA_iPace::OPERATING &&
            contactor_state != Contactormanager::INIT &&
            contactor_state != Contactormanager::FAULT)
        {
            state = OPERATING;
        }
        break;

    case OPERATING:
        break;

    case FAULT:
        // Stay in fault until power cycle; keep contactors as-is
        break;
    }
}

//###############################################################################################################################################################################
//  Battery Management Functions
//###############################################################################################################################################################################

void BMS::update_soc_ocv_lut()
{
    if (fabs(pack_current) <= BMS_OCV_CURRENT_THRESHOLD)
    {
        // Use lowest cell voltage across the pack as OCV reference
        const float ocv = batteryPack.get_lowest_cell_voltage();

        // Average pack temperature handled by BatteryPack
        const float avgTemp = batteryPack.get_average_temperature();

        soc_ocv_lut = SOC_FROM_OCV_TEMP(avgTemp, ocv);
    }
}

void BMS::update_soc_coulomb_counting()
{
    soc_coulomb_counting = 0.0f;
}

void BMS::correct_soc()
{
    soc = soc_ocv_lut;
}

void BMS::calculate_soh() {}

void BMS::lookup_current_limits()
{
    // Use the coldest cell temperature of the pack as conservative limit.
    // Assumption: The coldest cell determines the safe current limits for both
    // charging and discharging operations.
    float temperature = batteryPack.get_lowest_temperature();

    // Lookup tables defined in utils/current_limit_lookup.h
    float discharge_peak = DISCHARGE_PEAK_CURRENT_LIMIT(temperature);
    float discharge_cont = DISCHARGE_CONT_CURRENT_LIMIT(temperature);
    float charge_peak = CHARGE_PEAK_CURRENT_LIMIT(temperature);
    float charge_cont = CHARGE_CONT_CURRENT_LIMIT(temperature);

    // Store results separately for charge and discharge
    current_limit_peak_discharge = discharge_peak; // default discharge limit
    current_limit_rms_discharge = discharge_cont;
    current_limit_peak_charge = charge_peak;
    current_limit_rms_charge = charge_cont;
}

void BMS::lookup_internal_resistance_table() {}
void BMS::estimate_internal_resistance_online()
{
    const float current_threshold = IR_ESTIMATION_CURRENT_STEP_THRESHOLD;
    const float alpha = IR_ESTIMATION_ALPHA;

    static bool first_run = true;
    static float last_pack_current = 0.0f;
    static float last_cell_voltage[CELLS_PER_MODULE * MODULES_PER_PACK] = {0};

    // Gather current and voltages
    pack_current = shunt.getCurrent();

    for (int i = 0; i < CELLS_PER_MODULE * MODULES_PER_PACK; ++i)
    {
        float v;
        if (batteryPack.get_cell_voltage(i, v))
        {
            cell_voltage[i] = v;
        }
        else
        {
            cell_voltage[i] = 0.0f;
        }
    }

    if (first_run)
    {
        last_pack_current = pack_current;
        for (int i = 0; i < CELLS_PER_MODULE * MODULES_PER_PACK; ++i)
        {
            last_cell_voltage[i] = cell_voltage[i];
            internal_resistance_estimated_cells[i] = 0.0f;
        }
        first_run = false;
        return;
    }

    float deltaI = pack_current - last_pack_current;

    if (fabs(deltaI) > current_threshold)
    {
        for (int i = 0; i < CELLS_PER_MODULE * MODULES_PER_PACK; ++i)
        {
            float deltaV = cell_voltage[i] - last_cell_voltage[i];
            float ir_sample = (deltaI != 0.0f) ? deltaV / deltaI : 0.0f;
            internal_resistance_estimated_cells[i] =
                alpha * ir_sample + (1.0f - alpha) * internal_resistance_estimated_cells[i];
        }
    }

    last_pack_current = pack_current;
    for (int i = 0; i < CELLS_PER_MODULE * MODULES_PER_PACK; ++i)
    {
        last_cell_voltage[i] = cell_voltage[i];
    }

    // Compute average pack internal resistance from estimated cell values
    float sum_ir = 0.0f;
    for (int i = 0; i < CELLS_PER_MODULE * MODULES_PER_PACK; ++i)
    {
        sum_ir += internal_resistance_estimated_cells[i];
    }
    internal_resistance_estimated =
        sum_ir / static_cast<float>(CELLS_PER_MODULE * MODULES_PER_PACK);
}
void BMS::select_internal_resistance_used() {}

void BMS::calculate_voltage_derate()
{
    // Derate discharge current using the lowest cell voltage in the pack
    float low_voltage = batteryPack.get_lowest_cell_voltage();

    float discharge_derate = 1.0f;
    // Gradually reduce discharge capability as cells approach the minimum cutoff
    if (low_voltage < V_MIN_DERATE && low_voltage > V_MIN_CUTOFF)
    {
        discharge_derate = (low_voltage - V_MIN_CUTOFF) / (V_MIN_DERATE - V_MIN_CUTOFF);
    }
    else if (low_voltage <= V_MIN_CUTOFF)
    {
        discharge_derate = 0.0f; // Below cutoff, no discharge allowed
    }

    current_limit_rms_derated_discharge = current_limit_rms_discharge * discharge_derate;

    // Derate charge current using the highest cell voltage in the pack
    float high_voltage = batteryPack.get_highest_cell_voltage();

    float charge_derate = 1.0f;
    // Limit charging as cells get close to the maximum voltage threshold
    if (high_voltage > V_MAX_DERATE && high_voltage < V_MAX_CUTOFF)
    {
        charge_derate = (V_MAX_CUTOFF - high_voltage) / (V_MAX_CUTOFF - V_MAX_DERATE);
    }
    else if (high_voltage >= V_MAX_CUTOFF)
    {
        charge_derate = 0.0f; // Above cutoff, charging not allowed
    }

    current_limit_rms_derated_charge = current_limit_rms_charge * charge_derate;
}
void BMS::calculate_rms_ema() {}
void BMS::calculate_dynamic_voltage_limit() {}

void BMS::select_current_limit() {}

void BMS::select_limp_home() {}

void BMS::rate_limit_current() {}

void BMS::update_balancing()
{
    float lowestV = batteryPack.get_lowest_cell_voltage();
    float cellDelta = batteryPack.get_delta_cell_voltage();
    bool temp_ok = batteryPack.get_highest_temperature() < BALANCE_MAX_TEMP;
    bool vehicle_ok = (vehicle_state == STATE_CHARGE);

    if (vehicle_ok && temp_ok && lowestV > BALANCE_MIN_VOLTAGE)
    {
        if (cellDelta > BALANCE_DELTA_V)
        {
            batteryPack.set_balancing_voltage(lowestV + BALANCE_OFFSET_V);
            batteryPack.set_balancing_active(true);
            balancing_finished = false;
        }
        else
        {
            batteryPack.set_balancing_active(false);
            balancing_finished = true;
        }
    }
    else if (vehicle_ok && (!temp_ok || lowestV <= BALANCE_MIN_VOLTAGE))
    {
        batteryPack.set_balancing_active(false);
        balancing_finished = true;
    }
    else
    {
        // Vehicle state does not allow balancing - just ensure outputs are off
        batteryPack.set_balancing_active(false);
        // keep balancing_finished unchanged so the VCU knows balancing still
        // needs to occur
    }
}

//###############################################################################################################################################################################
//  CAN Messaging
//###############################################################################################################################################################################

// Read messages into modules and check alive
void BMS::read_message()
{
    CANMessage msg;

    if (ACAN_T4::BMS_CAN.receive(msg))
    {
        if (msg.id == BMS_VCU_MSG_ID && msg.len == 8)
        {
            uint8_t tmp[8];
            memcpy(tmp, msg.data, 8);
            tmp[4] = 0; // crc byte cleared
            uint8_t crc = can_crc8(tmp);
            if (crc == msg.data[4])
            {
                vehicle_state = static_cast<VehicleState>(msg.data[0]);
                ready_to_shutdown = msg.data[1];
                if (msg.data[2])
                    contactorManager.close();
                else
                    contactorManager.open();

                vcu_counter = msg.data[3] & 0x0F;
                last_vcu_msg = millis();
                vcu_timeout = false;
            }
        }
    }

    if ((millis() - last_vcu_msg) > BMS_VCU_TIMEOUT)
    {
        vcu_timeout = true;
    }
}

void BMS::send_battery_status_message()
{
    CANMessage msg;

    msg.id = BMS_MSG_VOLTAGE;
    msg.len = 8;
    uint16_t pack_v = (uint16_t)(batteryPack.get_pack_voltage() * 10.0f);
    uint16_t pack_i = (uint16_t)((shunt.getCurrent() * 10.0f) + 5000.0f);
    msg.data[0] = pack_v & 0xFF;
    msg.data[1] = pack_v >> 8;
    msg.data[2] = pack_i & 0xFF;
    msg.data[3] = pack_i >> 8;
    msg.data[4] = (uint8_t)(batteryPack.get_lowest_cell_voltage() * 50.0f);
    msg.data[5] = (uint8_t)(batteryPack.get_highest_cell_voltage() * 50.0f);
    msg.data[6] = msg1_counter & 0x0F;
    msg.data[7] = 0;
    msg.data[7] = can_crc8(msg.data);
    send_message(&msg);
    msg1_counter = (msg1_counter + 1) & 0x0F;

    msg.id = BMS_MSG_CELL_TEMP;
    uint8_t minT = (uint8_t)(batteryPack.get_lowest_temperature() + 40.0f);
    uint8_t maxT = (uint8_t)(batteryPack.get_highest_temperature() + 40.0f);
    uint8_t cellAvg = (uint8_t)(batteryPack.get_pack_voltage() / (MODULES_PER_PACK * CELLS_PER_MODULE) * 50.0f);
    uint8_t cellDelta = (uint8_t)(batteryPack.get_delta_cell_voltage() * 100.0f);
    uint16_t packPower = (uint16_t)((batteryPack.get_pack_voltage() * shunt.getCurrent() / 1000.0f) * 10.0f + 2000.0f);
    msg.data[0] = minT;
    msg.data[1] = maxT;
    msg.data[2] = cellAvg;
    msg.data[3] = cellDelta;
    msg.data[4] = packPower & 0xFF;
    msg.data[5] = packPower >> 8;
    msg.data[6] = msg2_counter & 0x0F;
    msg.data[7] = 0;
    msg.data[7] = can_crc8(msg.data);
    send_message(&msg);
    msg2_counter = (msg2_counter + 1) & 0x0F;

    msg.id = BMS_MSG_LIMITS;
    uint16_t maxD = (uint16_t)(max_discharge_current * 10.0f);
    uint16_t maxC = (uint16_t)(max_charge_current * 10.0f);
    msg.data[0] = maxD & 0xFF;
    msg.data[1] = maxD >> 8;
    msg.data[2] = maxC & 0xFF;
    msg.data[3] = maxC >> 8;
    msg.data[4] = static_cast<uint8_t>(contactorManager.getState());
    msg.data[5] = static_cast<uint8_t>(dtc);
    msg.data[6] = msg3_counter & 0x0F;
    msg.data[7] = 0;
    msg.data[7] = can_crc8(msg.data);
    send_message(&msg);
    msg3_counter = (msg3_counter + 1) & 0x0F;

    msg.id = BMS_MSG_SOC;
    uint16_t soc16 = (uint16_t)(soc * 100.0f);
    uint16_t soh16 = (uint16_t)(10000);
    msg.data[0] = soc16 & 0xFF;
    msg.data[1] = soc16 >> 8;
    msg.data[2] = soh16 & 0xFF;
    msg.data[3] = soh16 >> 8;
    if (balancing_finished)
        msg.data[4] = 2; // balanced/finished
    else if (batteryPack.get_balancing_active())
        msg.data[4] = 1; // actively balancing
    else
        msg.data[4] = 0; // idle
    msg.data[5] = state;
    msg.data[6] = msg4_counter & 0x0F;
    msg.data[7] = 0;
    msg.data[7] = can_crc8(msg.data);
    send_message(&msg);
    msg4_counter = (msg4_counter + 1) & 0x0F;

    msg.id = BMS_MSG_HMI;
    uint16_t energy = 0;
    uint16_t ttf = 0;
    msg.data[0] = energy & 0xFF;
    msg.data[1] = energy >> 8;
    msg.data[2] = ttf & 0xFF;
    msg.data[3] = ttf >> 8;
    msg.data[4] = msg5_counter & 0x0F;
    msg.data[5] = 0;
    msg.data[6] = 0;
    msg.data[7] = 0;
    msg.data[5] = can_crc8(msg.data);
    send_message(&msg);

    msg5_counter = (msg5_counter + 1) & 0x0F;
}

void BMS::send_message(CANMessage *frame)
{
    if (ACAN_T4::BMS_CAN.tryToSend(*frame))
    {
        // Serial.println("Send ok");
    }
    else
    {
        dtc = static_cast<DTC_BMS>(dtc | DTC_BMS_CAN_SEND_ERROR);
        // Serial.println("Send nok");
    }
}


