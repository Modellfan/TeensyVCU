#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery_manager.h"
#include "utils/can_packer.h"
#include <math.h>

#define DEBUG

BMS::BMS(BatteryPack &_batteryPack) : batteryPack(_batteryPack)
{
    state = INIT;
    dtc = DTC_BMS_NONE;
    moduleToBeMonitored = 0;
}

// void BMS::print()
// {
//     // Implementation for print method
// }

void BMS::initialize()
{
    // Set up CAN port
    ACAN_T4_Settings settings(500 * 1000); // 500 kbit/s
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
}

// Read messages into modules and check alive
void BMS::read_message()
{
    CANMessage msg;

    if (ACAN_T4::BMS_CAN.receive(msg))
    {
        //
    }

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
    //     // Additional fault handling logic can be added here if needed
    //     break;
    // }
    // }
}

// void BMS::Task2Ms()
// {
//     // Implementation for Task2Ms method

//     //Decode can messages. Get Max charge state
// }

// void BMS::Task10Ms()
// {
//     // Implementation for Task10Ms method
// }

// void BMS::Task100Ms()
// {
//     // Implementation for Task100Ms method
// }

void BMS::Monitor100Ms()
{
    CANMessage msg;

    msg.data64 = 0;
    msg.id = 1000; // Message 0x3e8 mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules2__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: -, None
    pack(msg, batteryPack.modules[2].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules2__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: -, None
    pack(msg, batteryPack.modules[2].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules2__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules2__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_balancing(0), 48, 1, false);                    // batteryPack__modules2__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(1), 49, 1, false);                    // batteryPack__modules2__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(2), 50, 1, false);                    // batteryPack__modules2__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(3), 51, 1, false);                    // batteryPack__modules2__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(4), 52, 1, false);                    // batteryPack__modules2__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(5), 53, 1, false);                    // batteryPack__modules2__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(6), 54, 1, false);                    // batteryPack__modules2__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(7), 55, 1, false);                    // batteryPack__modules2__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(8), 56, 1, false);                    // batteryPack__modules2__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(9), 57, 1, false);                    // batteryPack__modules2__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(10), 58, 1, false);                   // batteryPack__modules2__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    pack(msg, batteryPack.modules[2].get_balancing(11), 59, 1, false);                   // batteryPack__modules2__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: -40, unit: -, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1001; // Message 0x3e9 mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules2__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1002; // Message 0x3ea mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules2__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1003; // Message 0x3eb mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules2__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules2__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1004; // Message 0x3ec mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules2__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules2__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules2__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules2__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    // Serial.println(batteryPack.modules[2].get_temperature(0));
}

// void BMS::Monitor1000Ms()
// {
//     // Implementation for Monitor1000Ms method
// }

// void BMS::calculate_current_limits()
// {
//     float max_current_temperature_derating;
//     float max_current_dynamic_current_derating_battery;
//     float max_current_dynamic_current_derating_fuse;
//     float max_current_voltage_derating;
//     float max_current_cable_temperature_derating;
//     float max_current_limp_home;
//     float max_current_constant_max;

//     float max_current = max_current_constant_max;
//     max_current = fmin(max_current, max_current_temperature_derating);
//     max_current = fmin(max_current, max_current_dynamic_current_derating_battery);
//     max_current = fmin(max_current, max_current_dynamic_current_derating_fuse);
//     max_current = fmin(max_current, max_current_voltage_derating);
//     max_current = fmin(max_current, max_current_cable_temperature_derating);
//     max_current = fmin(max_current, max_current_limp_home);
//     //Aber auch mindestens Limp home strom

//     float min_current_temperature_derating;
//     float min_current_dynamic_current_derating_battery;
//     float min_current_dynamic_current_derating_fuse;
//     float min_current_voltage_derating;
//     float min_current_cable_temperature_derating;
//     float min_current_limp_home;
//     float min_current_constant_max;

//     float min_current = min_current_constant_max;
//     min_current = fmax(min_current, min_current_temperature_derating);
//     min_current = fmax(min_current, min_current_dynamic_current_derating_battery);
//     min_current = fmax(min_current, min_current_dynamic_current_derating_fuse);
//     min_current = fmax(min_current, min_current_voltage_derating);
//     min_current = fmax(min_current, min_current_cable_temperature_derating);
//     min_current = fmax(min_current, min_current_limp_home);
// };

// void BMS::calculate_balancing_target()
// {
//     // Implementation for calculate_balancing_target method
// }

// void BMS::calculate_soc()
// {
//     // Implementation for calculate_soc method
// }

// void BMS::calculate_soc_lut()
// {
//     // Implementation for calculate_soc_lut method
// }

// void BMS::calculate_soc_ekf()
// {
//     // Implementation for calculate_soc_ekf method
// }

// void BMS::calculate_soc_coulomb_counting()
// {
//     // Implementation for calculate_soc_coulomb_counting method
// }

// void BMS::calculate_open_circuit_voltage()
// {
//     // Implementation for calculate_open_circuit_voltage method
// }

// void BMS::calculate_soh()
// {
//     // Implementation for calculate_soh method
// }

// void BMS::send_outgoing_messages()
// {
//     // Implementation for send_outgoing_messages method

//     //Messages to charger
//     //-min current
//     //Messages to inverter
//     //-max current
//     //-min current
//     //-bms bollean failiure
// }

// void BMS::calculate_hmi_values()
// {
//     // Implementation for calculate_hmi_values method

//     // Temperature
//     // SOC
//     // SOE
// }

// void BMS::update_state_machine()
// {
//     // Implementation for update_state_machine method
//     //-open contcator when in charge mode failiure
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
    }
}

// const char *BatteryPack::getStateString()
// {
//     switch (state)
//     {
//     case INIT:
//         return "INIT";
//     case OPERATING:
//         return "OPERATING";
//     case FAULT:
//         return "FAULT";
//     default:
//         return "UNKNOWN STATE";
//     }
// }

// String BatteryPack::getDTCString()
// {
//     String errorString = "";

//     if (static_cast<uint8_t>(dtc) == 0)
//     {
//         errorString += "None";
//     }
//     else
//     {
//         if (dtc & DTC_PACK_CAN_SEND_ERROR)
//         {
//             errorString += "DTC_PACK_CAN_SEND_ERROR, ";
//         }
//         if (dtc & DTC_PACK_CAN_INIT_ERROR)
//         {
//             errorString += "DTC_PACK_CAN_INIT_ERROR, ";
//         }
//         if (dtc & DTC_PACK_MODULE_FAULT)
//         {
//             errorString += "DTC_PACK_MODULE_FAULT, ";
//         }

//         // Remove the trailing comma and space
//         errorString.remove(errorString.length() - 2);
//     }

//     return errorString;
// }
