#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery_manager.h"
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "utils/can_packer.h"
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
    current_limit_peak = 0.0f;
    current_limit_rms = 0.0f;
    current_limit_peak_charge = 0.0f;
    current_limit_rms_charge = 0.0f;
    current_limit_rms_derated_discharge = 0.0f;
    current_limit_rms_derated_charge = 0.0f;
    ready_to_shutdown = false;
    vehicle_state = STATE_SLEEP;
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
        dtc |= DTC_BMS_CAN_INIT_ERROR;
    }

    contactorManager.close();
}

void BMS::Task2Ms() { read_message(); } // Read Can messages ?
void BMS::Task10Ms()
{


}

void BMS::Task100Ms()
{

}

void BMS::Task1000Ms()
{

}

// Read messages into modules and check alive
void BMS::read_message()
{
    CANMessage msg;

    if (ACAN_T4::BMS_CAN.receive(msg))
    {
        if (msg.id == VCU_STATUS_MSG_ID && msg.len >= 1)
        {
            vehicle_state = static_cast<VehicleState>(msg.data[0]);
        }
        //Incoming from Main
        //  Energy state - struct
        //  Close contactor manually/ open boolean
    }


}

void BMS::send_battery_status_message()
{
    // Implementation for send_outgoing_messages method

    //Messages to charger
    //-min current
    //Messages to inverter
    //-max current
    //-min current
    //-bms bollean failiure

    //HMI & Main VCU
        //max temp °C
        //min temp °C
        //delimiting temp °C
        //soc %
        //soh %
        //remaining capacity Wh
        //total capacity Wh
        //Battery Voltage V
        //Battery Current A
        //Min Current A
        //Max Current A
        //Error State

            CANMessage msg;

    msg.data64 = 0;
    msg.id = 1050; // Message 0x41a pack_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.getState(), 0, 8, false, 1, 0);                    // batteryPack__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.getDTC(), 8, 8, false, 1, 0);                      // batteryPack__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.get_balancing_voltage(), 16, 16, false, 0.001, 0); // get_balancing_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_balancing_active(), 32, 1, false);             // batteryPack__modules7__get_balancing_active : 32|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.get_any_module_balancing(), 33, 1, false);         // batteryPack__modules7__get_any_module_balancing : 33|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, ready_to_shutdown, 34, 1, false);
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1051; // Message 0x41b pack_voltage 8bits None
    msg.len = 8;
    pack(msg, batteryPack.get_lowest_cell_voltage(), 0, 16, false, 0.001, 0);   // batteryPack__get_lowest_cell_voltage : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_highest_cell_voltage(), 16, 16, false, 0.001, 0); // batteryPack__get_highest_cell_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_pack_voltage(), 32, 16, false, 0.01, 0);         // batteryPack__get_pack_voltage : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_delta_cell_voltage(), 48, 16, false, 0.001, 0);   // batteryPack__get_delta_cell_voltage : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1052; // Message 0x41c pack_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.get_lowest_temperature(), 0, 16, false, 1, -40);   // batteryPack__get_lowest_temperature : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.get_highest_temperature(), 16, 16, false, 1, -40); // batteryPack__get_highest_temperature : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, max_discharge_current, 32, 16, false, 0.1, 0);
    pack(msg, max_charge_current, 48, 16, false, 0.1, 0);
    send_message(&msg);

}


// void BMS::update_state_machine()
// {

        // for (int i = 0; i < numModules; i++)
    // {
    //     modules[i].check_alive();
    // }

    // switch (this->state)
    // {
    // case INIT: // Wait for all modules to go into init
    // {
    //     byte numModulesOperating = 0;
    //     for (int i = 0; i < numModules; i++)
    //     {
    //         float v = modules[i].get_voltage();
    //         if (modules[i].getState() == BatteryModule::OPERATING)
    //         {
    //             numModulesOperating++;
    //         }
    //         if (modules[i].getState() == BatteryModule::FAULT)
    //         {
    //             dtc |= DTC_PACK_MODULE_FAULT;
    //         }
    //     }

    //     if (numModulesOperating == PACK_WAIT_FOR_NUM_MODULES)
    //     {
    //         state = OPERATING;
    //     }
    //     if (dtc > 0)
    //     {
    //         state = FAULT;
    //     }
    //     break;
    // }
    // case OPERATING: // Check if no module fault is popping up
    // {
    //     for (int i = 0; i < numModules; i++)
    //     {
    //         if (modules[i].getState() == BatteryModule::FAULT)
    //         {
    //             dtc |= DTC_PACK_MODULE_FAULT;
    //         }
    //     }
    //     if (dtc > 0)
    //     {
    //         state = FAULT;
    //     }
    //     break;
    // }
    // case FAULT:
    // {
    //     // Additional fault handling logic can be added here if neededF 
    //     break;
    // }
    // }
// }

void BMS::send_message(CANMessage *frame)
{
    if (ACAN_T4::BMS_CAN.tryToSend(*frame))
    {
        // Serial.println("Send ok");
    }
    else
    {
        dtc |= DTC_BMS_CAN_SEND_ERROR;
        // Serial.println("Send nok");
    }
}

// --- Core Algorithm Stubs -------------------------------------------------

void BMS::update_soc_ocv_lut() {
    if (fabs(pack_current) <= BMS_OCV_CURRENT_THRESHOLD) {
        // Use lowest cell voltage across the pack as OCV reference
        const float ocv = batteryPack.get_lowest_cell_voltage();

        // Average pack temperature handled by BatteryPack
        const float avgTemp = batteryPack.get_average_temperature();

        soc_ocv_lut = SOC_FROM_OCV_TEMP(avgTemp, ocv);
    }
}

void BMS::update_soc_coulomb_counting() {
    soc_coulomb_counting = 0.0f;
}

void BMS::correct_soc() {
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
    current_limit_peak = discharge_peak; // default discharge limit
    current_limit_rms = discharge_cont;
    current_limit_peak_charge = charge_peak;
    current_limit_rms_charge = charge_cont;

    max_discharge_current = discharge_peak;
    max_charge_current = charge_peak;
}
void BMS::lookup_internal_resistance_table() {}
void BMS::estimate_internal_resistance_online() {}
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

    current_limit_rms_derated_discharge = current_limit_rms * discharge_derate;

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

void BMS::calculate_limp_home_limit() {}
void BMS::select_limp_home() {}

void BMS::rate_limit_current() {}

