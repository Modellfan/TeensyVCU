/*
 * This file is part of the ev mustang bms project.
 *
 * Copyright (C) 2022 Christian Kelly <chrskly@chrskly.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*

    MODULE

*/

#include <stdio.h>
#include "module.h"
#include "pack.h"
#include "utils/signalManager.h"
#include "utils/can_packer.h"
#include <ACAN_T4.h>

BatteryModule::BatteryModule() {}

BatteryModule::BatteryModule(int _id, BatteryPack *_pack)
{
    // printf("Creating module (id:%d, pack:%d, cpm:%d, t:%d)\n", _id, _pack->id, _numCells, _numTemperatureSensors);
    id = _id;

    // Point back to parent pack
    pack = _pack;

    // Initialise all cell voltages to zero
    numCells = 12;

    for (int c = 0; c < numCells; c++)
    {
        cellVoltage[c] = 0.000f;
        cellBalance[c] = 0;
        balanceDirection[c] = 0;
    }

    moduleVoltage = 0.000f;

    // Initialise temperature sensor readings to zero
    numTemperatureSensors = 4;

    for (int t = 0; t < numTemperatureSensors; t++)
    {
        cellTemperature[t] = -50.000f;
    }
    temperatureInternal = -50.000f;

    cmuError = false;
    state = INIT;
    dtc = DTC_CMU_NONE;

    lastUpdate = millis();
}

void BatteryModule::printFrame(CANMessage &frame)
{
    // Print message
    Serial.print("ID: ");
    Serial.print(frame.id, HEX);
    Serial.print(" Ext: ");
    if (frame.ext)
    {
        Serial.print("Y");
    }
    else
    {
        Serial.print("N");
    }
    Serial.print(" Len: ");
    Serial.print(frame.len, DEC);
    Serial.print(" ");
    for (int i = 0; i < frame.len; i++)
    {
        Serial.print(frame.data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}
void BatteryModule::print()
{
    // Serial.println(static_cast<uint8_t>(dtc));
    Serial.printf("Module id: %d (state: %s; DTC: %s; Module Voltage %fV)\n", id, getStateString(), getDTCString().c_str(), moduleVoltage);
    Serial.printf("        Cell Voltages : ");
    for (int c = 0; c < numCells; c++)
    {
        Serial.printf("%d:%1.3fV B%d ", c, cellVoltage[c], cellBalance[c]);
    }
    Serial.printf("\n");
    Serial.printf("        Temperatures : ");
    for (int t = 0; t < numTemperatureSensors; t++)
    {
        Serial.printf("%d:%3.2fC ", t, cellTemperature[t]);
    }
    Serial.printf("\n");

    // SignalManager::logSignal("sg_mod" + String(id) + "_alive", module_is_alive());
    //  SignalManager::logSignal("sg_mod" + String(id) + "_populated", all_module_data_populated());
    for (int c = 0; c < numCells; c++)
    {
        SignalManager::logSignal("sg_mod" + String(id) + "_cell" + String(c) + "_voltage", cellVoltage[c]);
        SignalManager::logSignal("sg_mod" + String(id) + "_cell" + String(c) + "_balancing", cellBalance[c]);
    }
    for (int t = 0; t < numTemperatureSensors; t++)
    {
        SignalManager::logSignal("sg_mod" + String(id) + "_temperature" + String(t), cellTemperature[t]);
    }
}

void BatteryModule::process_message(CANMessage &msg)
{
    if ((msg.id & 0x00F) != id) // check if module id belongs to this module
    {
        return;
    }
    else
    {
        lastUpdate = millis();
    }

    switch (msg.id & 0x0F0) // removes the module spicif part of the message id
    {
    case 0x0:                                             // Message 0x105 CMU0ErrorBalanceStatus 8bits 50ms
        cmuError = unpack(msg, 0, 32, false);             // cmuError : 0|32 little_endian unsigned scale: 1, offset: 0, unit: None, None
        balanceDirection[0] = unpack(msg, 32, 1, false);  // balanceDirection0 : 32|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[1] = unpack(msg, 33, 1, false);  // balanceDirection1 : 33|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[2] = unpack(msg, 34, 1, false);  // balanceDirection2 : 34|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[3] = unpack(msg, 35, 1, false);  // balanceDirection3 : 35|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[4] = unpack(msg, 36, 1, false);  // balanceDirection4 : 36|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[5] = unpack(msg, 37, 1, false);  // balanceDirection5 : 37|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[6] = unpack(msg, 38, 1, false);  // balanceDirection6 : 38|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[7] = unpack(msg, 39, 1, false);  // balanceDirection7 : 39|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[8] = unpack(msg, 40, 1, false);  // balanceDirection8 : 40|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[9] = unpack(msg, 41, 1, false);  // balanceDirection9 : 41|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[10] = unpack(msg, 42, 1, false); // balanceDirection10 : 42|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        balanceDirection[11] = unpack(msg, 43, 1, false); // balanceDirection11 : 43|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Charge;_1=Discharge
        // Counter_x105 = unpack(msg, 52, 4, false, 1, 0);   // Counter_x105 : 52|4 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_x105 = unpack(msg, 56, 8, false, 1, 0);       // CRC_x105 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    case 0x20:                                                 // Message 0x125 CMU_0x125_Voltage_0_2 8bits 100ms
        cellVoltage[0] = unpack(msg, 0, 15, false, 0.001, 0);  // cellVoltage0 : 0|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[0] = unpack(msg, 15, 1, false);            // cellBalance1 : 15|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[1] = unpack(msg, 16, 15, false, 0.001, 0); // cellVoltage1 : 16|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[1] = unpack(msg, 31, 1, false);            // cellBalance2 : 31|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[2] = unpack(msg, 32, 15, false, 0.001, 0); // cellVoltage2 : 32|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[2] = unpack(msg, 47, 1, false);            // cellBalance3 : 47|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // Counter_0x125 = unpack(msg, 52, 4, false, 1, 0);     // Counter_0x125 : 52|4 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_0x125 = unpack(msg, 56, 8, false, 1, 0);         // CRC_0x125 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    case 0x30:                                                 // Message 0x135 CMU_0x135_Voltage_3_5 8bits 100ms
        cellVoltage[3] = unpack(msg, 0, 15, false, 0.001, 0);  // cellVoltage3 : 0|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[3] = unpack(msg, 15, 1, false);            // cellBalance3 : 15|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[4] = unpack(msg, 16, 15, false, 0.001, 0); // cellVoltage4 : 16|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[4] = unpack(msg, 31, 1, false);            // cellBalance4 : 31|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[5] = unpack(msg, 32, 15, false, 0.001, 0); // cellVoltage5 : 32|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[5] = unpack(msg, 47, 1, false);            // cellBalance5 : 47|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        // Counter_0x135 = unpack(msg, 52, 4, false, 1, 0);     // Counter_0x135 : 52|4 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_0x135 = unpack(msg, 56, 8, false, 1, 0);         // CRC_0x135 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    case 0x40:                                                 // Message 0x145 CMU_0x145_Voltage_6_8 8bits 100ms
        cellVoltage[6] = unpack(msg, 0, 15, false, 0.001, 0);  // CellVoltage6 : 0|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[6] = unpack(msg, 15, 1, false);            // cellBalance6 : 15|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[7] = unpack(msg, 16, 15, false, 0.001, 0); // CellVoltage7 : 16|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[7] = unpack(msg, 31, 1, false);            // cellBalance7 : 31|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[8] = unpack(msg, 32, 15, false, 0.001, 0); // CellVoltage8 : 32|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[8] = unpack(msg, 47, 1, false);            // cellBalance8 : 47|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        // Counter_0x145 = unpack(msg, 52, 4, false, 1, 0);     // Counter_0x145 : 52|4 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_0x145 = unpack(msg, 56, 8, false, 1, 0);         // CRC_0x145 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    case 0x50:                                                  // Message 0x155 CMU_0x155_Voltage_9_11 8bits 100ms
        cellVoltage[9] = unpack(msg, 0, 15, false, 0.001, 0);   // CellVoltage9 : 0|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[9] = unpack(msg, 15, 1, false);             // cellBalance9 : 15|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[10] = unpack(msg, 16, 15, false, 0.001, 0); // CellVoltage10 : 16|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[10] = unpack(msg, 31, 1, false);            // cellBalance10 : 31|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        cellVoltage[11] = unpack(msg, 32, 15, false, 0.001, 0); // CellVoltage11 : 32|15 little_endian unsigned scale: 0.001, offset: 0, unit: V, None
        cellBalance[11] = unpack(msg, 47, 1, false);            // cellBalance11 : 47|1 little_endian unsigned scale: 1, offset: 0, unit: None, 0=Balance_Inactive;_1=Balance_Active
        // Counter_0x155 = unpack(msg, 55, 1, false);            // Counter_0x155 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_0x155 = unpack(msg, 56, 8, false, 1, 0);          // CRC_0x155 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    case 0x60:                                               // Message 0x165 CMU_0x165_Total_Voltage 8bits 100ms
        moduleVoltage = unpack(msg, 0, 16, false, 0.001, 0); // moduleVoltage : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: None, None
        // Counter_0x165 = unpack(msg, 52, 4, false, 1, 0);     // Counter_0x165 : 52|4 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_0x165 = unpack(msg, 56, 8, false, 1, 0);         // CRC_0x165 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    case 0x70:                                                   // Message 0x175 CMU_0x175_Temperatures 8bits 100ms
        cellTemperature[0] = unpack(msg, 0, 8, false, 1, -40);   // temperature0 : 0|8 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
        cellTemperature[1] = unpack(msg, 8, 8, false, 1, -40);   // temperature1 : 8|8 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
        cellTemperature[2] = unpack(msg, 16, 8, false, 1, -40);  // temperature2 : 16|8 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
        cellTemperature[3] = unpack(msg, 24, 8, false, 1, -40);  // temperature3 : 24|8 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
        temperatureInternal = unpack(msg, 32, 8, false, 1, -40); // temperatureInternal : 32|8 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
        // Counter_0x175 = unpack(msg, 52, 4, false, 1, 0);         // Counter_0x175 : 52|4 little_endian unsigned scale: 1, offset: 0, unit: None, None
        // CRC_0x15 = unpack(msg, 56, 8, false, 1, 0);              // CRC_0x15 : 56|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
        break;

    default:
        break;
    }

    // Statemaschine
    switch (state)
    {
    case INIT:
    {
        if (check_if_module_data_is_populated() && (!cmuError))
        {
            state = OPERATING;
            // Serial.println(String(millis()) + ": Now in operation");
        }
        // if (cmuError)
        // { // CMU Error is normal on startup before receiving the right messages
        //     state = FAULT;
        //     dtc |= DTC_CMU_INTERNAL_ERROR;
        // }
        break;
    }
    case OPERATING:
    {
        // We check for all errors to come
        if (cmuError)
        {
            state = FAULT;
            dtc |= DTC_CMU_INTERNAL_ERROR;
        }
        if (temperatureInternal > CMU_MAX_INTERNAL_WARNING_TEMPERATURE)
        {
            state = FAULT;
            dtc |= DTC_CMU_TEMPERATURE_TOO_HIGH;
        }
        if (plausibilityCheck() == false)
        {
            state = FAULT;
        }
        break;
    }
    case FAULT:
    {
        // We check for more errors to come. No routine implemented to heal fault state except power off.
        if (cmuError)
        {
            state = FAULT;
            dtc |= DTC_CMU_INTERNAL_ERROR;
        }
        if (temperatureInternal > CMU_MAX_INTERNAL_WARNING_TEMPERATURE)
        {
            state = FAULT;
            dtc |= DTC_CMU_TEMPERATURE_TOO_HIGH;
        }
        if (plausibilityCheck() == false)
        {
            state = FAULT;
        }
        break;
    }
    }
}

// Return total module voltage by summing the cell voltages
float BatteryModule::get_voltage()
{
    return moduleVoltage;
}

// Return the voltage of the lowest cell voltage in the module
float BatteryModule::get_lowest_cell_voltage()
{
    float lowestCellVoltage = 10.0000f;
    for (int c = 0; c < numCells; c++)
    {
        // printf("Comparing %3.3f and %3.3f\n", cellVoltage[c], lowestCellVoltage);
        if (cellVoltage[c] < lowestCellVoltage)
        {
            lowestCellVoltage = cellVoltage[c];
        }
    }
    return lowestCellVoltage;
}

// Return the voltage of the highest cell in the module
float BatteryModule::get_highest_cell_voltage()
{
    float highestCellVoltage = 0.000f;
    for (int c = 0; c < numCells; c++)
    {
        // printf("module : comparing : %.4f to %.4f\n", cellVoltage[c], highestCellVoltage);
        if (cellVoltage[c] > highestCellVoltage)
        {
            highestCellVoltage = cellVoltage[c];
        }
    }
    return highestCellVoltage;
}

// Check on startup, if all values are populated
bool BatteryModule::check_if_module_data_is_populated()
{
    bool voltageMissing = false;
    for (int c = 0; c < numCells; c++)
    {
        if (cellVoltage[c] == 0.000f)
        {
            voltageMissing = true;
        }
    }
    bool temperatureMissing = false;
    for (int t = 0; t < numTemperatureSensors; t++)
    {
        if (cellTemperature[t] == -50.000f)
        {
            temperatureMissing = true;
        }
    }
    return (!voltageMissing) && (!temperatureMissing) && (moduleVoltage != -50.000f);
}

// Return the temperature of the coldest sensor in the module
float BatteryModule::get_lowest_temperature()
{
    float lowestTemperature = 1000.0f;
    for (int t = 0; t < numTemperatureSensors; t++)
    {
        if (cellTemperature[t] < lowestTemperature)
        {
            lowestTemperature = cellTemperature[t];
        }
    }
    return lowestTemperature;
}

// Return the temperature of the hottest sensor in the module
float BatteryModule::get_highest_temperature()
{
    float highestTemperature = -50;
    for (int t = 0; t < numTemperatureSensors; t++)
    {
        if (cellTemperature[t] > highestTemperature)
        {
            highestTemperature = cellTemperature[t];
        }
    }
    return highestTemperature;
}

// Check module alive cannot be in the same runable as process message. becaus ethis is only called, when a message is available.
void BatteryModule::check_alive()
{
    // Statemaschine
    switch (state)
    {
    case INIT:
        break;
    case OPERATING:
        if ((millis() - lastUpdate) > PACK_ALIVE_TIMEOUT)
        {
            state = FAULT;
            dtc |= DTC_CMU_TIMED_OUT;
            // Serial.println(String(millis()) + ": Timed out");
        }
        break;
    case FAULT:
        break;
    }
}

bool BatteryModule::plausibilityCheck()
{
    bool plausible = true;
    if ((get_lowest_cell_voltage() < CMU_MIN_PLAUSIBLE_VOLTAGE) || (get_highest_cell_voltage() > CMU_MAX_PLAUSIBLE_VOLTAGE))
    {
        plausible = false;
        dtc |= DTC_CMU_SINGLE_VOLTAGE_IMPLAUSIBLE;
    }
    if ((get_lowest_temperature() < CMU_MIN_PLAUSIBLE_TEMPERATURE) || (get_highest_temperature() > CMU_MAX_PLAUSIBLE_TEMPERATURE))
    {
        plausible = false;
        dtc |= DTC_CMU_TEMPERATURE_IMPLAUSIBLE;
    }

    float totalVoltage = 0.0000f;
    for (int c = 0; c < numCells; c++)
    {
        totalVoltage += cellVoltage[c];
    }

    if (((totalVoltage - CMU_MAX_DELTA_MODULE_CELL_VOLTAGE) > moduleVoltage) || ((totalVoltage + CMU_MAX_DELTA_MODULE_CELL_VOLTAGE) < moduleVoltage))
    {
        plausible = false;
        dtc |= DTC_CMU_MODULE_VOLTAGE_IMPLAUSIBLE;
    }
    return plausible;
}

const char *BatteryModule::getStateString()
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

BatteryModule::STATE_CMU BatteryModule::getState() { return state; }

float BatteryModule::get_average_temperature()
{
    float sumTemperature = 0.0f;

    for (int t = 0; t < numTemperatureSensors; t++)
    {
        sumTemperature += cellTemperature[t];
    }

    // Calculate the average temperature
    float averageTemperature = sumTemperature / numTemperatureSensors;

    return averageTemperature;
}

String BatteryModule::getDTCString()
{
    String errorString = "";

    if (static_cast<uint8_t>(dtc) == 0)
    {
        errorString += "None";
    }
    else
    {
        if (dtc & DTC_CMU_INTERNAL_ERROR)
        {
            errorString += "DTC_CMU_INTERNAL_ERROR, ";
        }
        if (dtc & DTC_CMU_TEMPERATURE_TOO_HIGH)
        {
            errorString += "DTC_CMU_TEMPERATURE_TOO_HIGH, ";
        }
        if (dtc & DTC_CMU_SINGLE_VOLTAGE_IMPLAUSIBLE)
        {
            errorString += "DTC_CMU_SINGLE_VOLTAGE_IMPLAUSIBLE, ";
        }
        if (dtc & DTC_CMU_TEMPERATURE_IMPLAUSIBLE)
        {
            errorString += "DTC_CMU_TEMPERATURE_IMPLAUSIBLE, ";
        }
        if (dtc & DTC_CMU_TIMED_OUT)
        {
            errorString += "DTC_CMU_TIMED_OUT, ";
        }
        if (dtc & DTC_CMU_MODULE_VOLTAGE_IMPLAUSIBLE)
        {
            errorString += "DTC_CMU_MODULE_VOLTAGE_IMPLAUSIBLE, ";
        }

        // Remove the trailing comma and space
        errorString.remove(errorString.length() - 2);
    }
    return errorString;
}

float BatteryModule::get_cell_voltage(byte cellIndex)
{
    // Check if the cell index is valid
    if (cellIndex < 0 || cellIndex >= numCells)
    {
        // Invalid cell index, return 0.0 as an indication of an error
        return 0.0;
    }

    // Get the voltage of the specified cell
    return cellVoltage[cellIndex];
}

bool BatteryModule::get_is_balancing()
{
    for (int i = 0; i < numCells; i++)
    {
        if (cellBalance[i])
        {
            // If any cell is balancing, return true
            return true;
        }
    }

    // If none of the cells is balancing, return false
    return false;
}
