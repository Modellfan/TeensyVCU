#include "BMS.h"
#include <math.h>

BMS::BMS(BatteryPack &_batteryPack) : batteryPack(_batteryPack)
{
    state = INIT;
    dtc = DTC_PACK_NONE;
}

void BMS::print()
{
    // Implementation for print method
}

void BMS::initialize()
{
    // Implementation for initialize method
}

void BMS::Task2Ms()
{
    // Implementation for Task2Ms method

    //Decode can messages. Get Max charge state
}

void BMS::Task10Ms()
{
    // Implementation for Task10Ms method
}

void BMS::Task100Ms()
{
    // Implementation for Task100Ms method
}

void BMS::Monitor100Ms()
{
    // Implementation for Monitor100Ms method
}

void BMS::Monitor1000Ms()
{
    // Implementation for Monitor1000Ms method
}

void BMS::calculate_current_limits()
{
    float max_current_temperature_derating;
    float max_current_dynamic_current_derating_battery;
    float max_current_dynamic_current_derating_fuse;
    float max_current_voltage_derating;
    float max_current_cable_temperature_derating;
    float max_current_limp_home;
    float max_current_constant_max;


    float max_current = max_current_constant_max;
    max_current = fmin(max_current, max_current_temperature_derating);
    max_current = fmin(max_current, max_current_dynamic_current_derating_battery);
    max_current = fmin(max_current, max_current_dynamic_current_derating_fuse);
    max_current = fmin(max_current, max_current_voltage_derating);
    max_current = fmin(max_current, max_current_cable_temperature_derating);
    max_current = fmin(max_current, max_current_limp_home);
    //Aber auch mindestens Limp home strom

    float min_current_temperature_derating;
    float min_current_dynamic_current_derating_battery;
    float min_current_dynamic_current_derating_fuse;
    float min_current_voltage_derating;
    float min_current_cable_temperature_derating;
    float min_current_limp_home;
    float min_current_constant_max;

    float min_current = min_current_constant_max;
    min_current = fmax(min_current, min_current_temperature_derating);
    min_current = fmax(min_current, min_current_dynamic_current_derating_battery);
    min_current = fmax(min_current, min_current_dynamic_current_derating_fuse);
    min_current = fmax(min_current, min_current_voltage_derating);
    min_current = fmax(min_current, min_current_cable_temperature_derating);
    min_current = fmax(min_current, min_current_limp_home);
};


void BMS::calculate_balancing_target()
{
    // Implementation for calculate_balancing_target method
}

void BMS::calculate_soc()
{
    // Implementation for calculate_soc method
}

void BMS::calculate_soc_lut()
{
    // Implementation for calculate_soc_lut method
}

void BMS::calculate_soc_ekf()
{
    // Implementation for calculate_soc_ekf method
}

void BMS::calculate_soc_coulomb_counting()
{
    // Implementation for calculate_soc_coulomb_counting method
}

void BMS::calculate_open_circuit_voltage()
{
    // Implementation for calculate_open_circuit_voltage method
}

void BMS::calculate_soh()
{
    // Implementation for calculate_soh method
}

void BMS::send_outgoing_messages()
{
    // Implementation for send_outgoing_messages method

    //Messages to charger
    //-min current
    //Messages to inverter
    //-max current
    //-min current
    //-bms bollean failiure
}

void BMS::calculate_hmi_values()
{
    // Implementation for calculate_hmi_values method

    // Temperature
    // SOC
    // SOE
}

void BMS::update_state_machine()
{
    // Implementation for update_state_machine method
    //-open contcator when in charge mode failiure
}

void BMS::send_message(CANMessage *frame)
{
    // Implementation for send_message method
}

const char *BatteryPack::getStateString()
{
    switch (state)
    {
    case INIT:
        return "INIT";
    case OPERATING:
        return "OPERATING";
    case FAULT:
        return "FAULT";
    default:
        return "UNKNOWN STATE";
    }
}

String BatteryPack::getDTCString()
{
    String errorString = "";

    if (static_cast<uint8_t>(dtc) == 0)
    {
        errorString += "None";
    }
    else
    {
        if (dtc & DTC_PACK_CAN_SEND_ERROR)
        {
            errorString += "DTC_PACK_CAN_SEND_ERROR, ";
        }
        if (dtc & DTC_PACK_CAN_INIT_ERROR)
        {
            errorString += "DTC_PACK_CAN_INIT_ERROR, ";
        }
        if (dtc & DTC_PACK_MODULE_FAULT)
        {
            errorString += "DTC_PACK_MODULE_FAULT, ";
        }

        // Remove the trailing comma and space
        errorString.remove(errorString.length() - 2);
    }

    return errorString;
}
