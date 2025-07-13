#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery_manager.h"
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "utils/can_packer.h"
#include <math.h>

// #define DEBUG

BMS::BMS(BatteryPack &_batteryPack, Shunt_ISA_iPace &_shunt, Contactormanager &_contactorManager) : batteryPack(_batteryPack), shunt(_shunt), contactorManager(_contactorManager)
{
    state = INIT;
    dtc = DTC_BMS_NONE;
    moduleToBeMonitored = 0;
    max_charge_current = 0.0f;
    max_discharge_current = 0.0f;
    ready_to_shutdown = false;
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
    float _current;
    // todo Check if current sensor state is operation
    _current = shunt.getCurrent();
    power = _current * batteryPack.get_pack_voltage();

    float _temp;
    _temp = shunt.getAmpereSeconds();
    ampere_seconds += _temp;
    watt_seconds += _temp * batteryPack.get_pack_voltage();

    //todo calculate from internal resistance the internal watt seconds

    soc_coulomb_counting = watt_seconds / total_capacity * 100; // SOC percentage

}

void BMS::Task100Ms()
{
    float _temperature;
    float _voltage;
    float _current;

    // todo Check if current sensor state is operation
    // maybe load balance this task by doing one module every 10ms

    _current = shunt.getCurrent();

    for (int i = 0; i <= CELLS_PER_MODULE * MODULES_PER_PACK; ++i)
    {
        if (batteryPack.get_cell_voltage(i, _voltage) == false)
        {
            cell_available[i] = false;
            continue;
        }
        if (batteryPack.get_cell_temperature(i, _temperature) == false)
        {
            cell_available[i] = false;
            continue;
        }
        // if cell value is fine
        internal_resistance[i] = 1.0; // make it a better calculation. in mOhm
        open_circuite_voltage[i] = _voltage - internal_resistance[i] * _current / 1000.0;
    }
}

// Read messages into modules and check alive
void BMS::read_message()
{
    CANMessage msg;

    if (ACAN_T4::BMS_CAN.receive(msg))
    {
        //Incoming from Main
        //  Energy state - struct
        //  Close contactor manually/ open boolean
    }


}

void BMS::Monitor100Ms()
{

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

}

// void BMS::calculate_hmi_values()
// {
//     // Implementation for calculate_hmi_values method

//     // Temperature
//     // SOC
//     // SOE
// }

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
    //     // Additional fault handling logic can be added here if needed
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

void BMS::set_max_charge_current(float current)
{
    max_charge_current = current;
}

void BMS::set_max_discharge_current(float current)
{
    max_discharge_current = current;
}

float BMS::get_max_charge_current() const
{
    return max_charge_current;
}

float BMS::get_max_discharge_current() const
{
    return max_discharge_current;
}

void BMS::set_ready_to_shutdown(bool ready)
{
    ready_to_shutdown = ready;
}

bool BMS::get_ready_to_shutdown() const
{
    return ready_to_shutdown;
}


//Interpolation

// Map2D<8, int16_t, int8_t> test;
// test.setXs_P(xs);
// test.setYs_P(ys);

// for (int idx = 250; idx < 2550; idx += 50)
// {
//   int8_t val = test.f(idx);
//   Serial.print(idx);
//   Serial.print(F(": "));
//   Serial.println((int)val);
// }

// const int16_t xs[] PROGMEM = {300, 700, 800, 900, 1500, 1800, 2100, 2500};
// const int8_t ys[] PROGMEM = {-127, -50, 127, 0, 10, -30, -50, 10};
// const byte ysb[] PROGMEM = {0, 30, 55, 89, 99, 145, 255, 10};
// const float ysfl[] PROGMEM = {-127.3, -49.9, 127.0, 0.0, 13.3, -33.0, -35.8, 10.0};
