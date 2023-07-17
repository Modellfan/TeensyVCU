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

#include <ACAN_T4.h>
#include <Arduino.h>

#include "module.h"
#include "settings.h"
#include "CRC8.h"
#include "pack.h"

BatteryPack::BatteryPack() {}

BatteryPack::BatteryPack(int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule)
{

    printf("Initialising BatteryPack %d\n", 1);

    numModules = _numModules;
    numCellsPerModule = _numCellsPerModule;
    numTemperatureSensorsPerModule = _numTemperatureSensorsPerModule;

    // Initialise modules
    for (int m = 0; m < numModules; m++)
    {
        // printf("Creating module %d (cpm:%d, tpm:%d)\n", m, numCellsPerModule, numTemperatureSensorsPerModule);
        modules[m] = BatteryModule(m, this, numCellsPerModule, numTemperatureSensorsPerModule);
    }

    // Set up CAN port
    // Serial.println("Battery CAN Initialize");
    // ACAN_T4_Settings settings(500 * 1000); // 125 kbit/s
    // const uint32_t errorCode = ACAN_T4::BATTERY_CAN.begin(settings);
    // Serial.print("Bitrate prescaler: ");
    // Serial.println(settings.mBitRatePrescaler);
    // Serial.print("Propagation Segment: ");
    // Serial.println(settings.mPropagationSegment);
    // Serial.print("Phase segment 1: ");
    // Serial.println(settings.mPhaseSegment1);
    // Serial.print("Phase segment 2: ");
    // Serial.println(settings.mPhaseSegment2);
    // Serial.print("RJW: ");
    // Serial.println(settings.mRJW);
    // Serial.print("Triple Sampling: ");
    // Serial.println(settings.mTripleSampling ? "yes" : "no");
    // Serial.print("Actual bitrate: ");
    // Serial.print(settings.actualBitRate());
    // Serial.println(" bit/s");
    // Serial.print("Exact bitrate ? ");
    // Serial.println(settings.exactBitRate() ? "yes" : "no");
    // Serial.print("Distance from wished bitrate: ");
    // Serial.print(settings.ppmFromWishedBitRate());
    // Serial.println(" ppm");
    // Serial.print("Sample point: ");
    // Serial.print(settings.samplePointFromBitStart());
    // Serial.println("%");
    // if (0 == errorCode)
    // {
    //     Serial.println("Battery Shunt CAN ok");
    // }
    // else
    // {
    //     Serial.print("Error Battery Shunt CAN: 0x");
    //     Serial.println(errorCode, HEX);
    // }

    // Set last update time to now
    lastUpdate = millis();
    voltage = 0.0000f;
    cellDelta = 0;

    // Set next balance time to 10 seconds from now
    nextBalanceTime = millis() + 10000;

    inStartup = true;
    modulePollingCycle = 0;

    crc8.begin();

    printf("Pack %d setup complete\n", 1);
}

void BatteryPack::print()
{
    printf("Pack %d : %3.2fV : %dmV\n", 1, voltage, cellDelta);
}

uint8_t BatteryPack::getcheck(CANMessage &msg, int id)
{
    unsigned char canmes[11];
    int meslen = msg.len + 1; // remove one for crc and add two for id bytes
    canmes[1] = msg.id;
    canmes[0] = msg.id >> 8;

    for (int i = 0; i < (msg.len - 1); i++)
    {
        canmes[i + 2] = msg.data[i];
    }
    return (crc8.get_crc8(canmes, meslen, finalxor[id]));
}

/*
 *
 * Contents of message
 *   byte 0 : balance data
 *   byte 1 : balance data
 *   byte 2 : 0x00
 *   byte 3 : 0x00
 *   byte 4 :
 *   byte 5 : 0x01
 *   byte 6 :
 *   byte 7 : checksum
 */

void BatteryPack::request_data()
{

    if (modulePollingCycle == 0xF)
    {
        modulePollingCycle = 0;
    }

    for (int m = 0; m < numModules; m++)
    {
        pollModuleFrame.id = 0x080 | (m);
        pollModuleFrame.len = 8;
        pollModuleFrame.data[0] = 0xC7;
        pollModuleFrame.data[1] = 0x10;
        pollModuleFrame.data[2] = 0x00;
        pollModuleFrame.data[3] = 0x00;
        if (inStartup)
        {
            pollModuleFrame.data[4] = 0x20;
            pollModuleFrame.data[5] = 0x00;
        }
        else
        {
            pollModuleFrame.data[4] = 0x40;
            pollModuleFrame.data[5] = 0x01;
        }
        pollModuleFrame.data[6] = modulePollingCycle << 4;
        if (inStartup && (modulePollingCycle == 2))
        {
            pollModuleFrame.data[6] = pollModuleFrame.data[6] + 0x04;
        }
        pollModuleFrame.data[7] = getcheck(pollModuleFrame, m);
        send_message(&pollModuleFrame);
    }

    if (inStartup && (modulePollingCycle == 2))
    {
        inStartup = false;
    }

    modulePollingCycle++;

    return;
}

void BatteryPack::read_message()
{

    // printf("Pack %d : checking for messages\n", id);

    CANMessage frame;

    if (ACAN_T4::BATTERY_CAN.receive(frame))
    {

        
        printf("Pack %d received message : id:%02X : ", 1, frame.id);
        for ( int i = 0; i < frame.len; i++ ) {
            printf("%02X ", frame.data[i]);
        }
        printf("\n");
        

        // Temperature messages
        if ((frame.id & 0xFF0) == 0x180)
        {
            decode_temperatures(&frame);
            // state(E_TEMPERATURE_UPDATE);
        }

        // Voltage messages
        if (frame.id > 0x99 && frame.id < 0x180)
        {
            decode_voltages(&frame);
            // state(E_CELL_VOLTAGE_UPDATE);
        }
    }
}

bool BatteryPack::pack_is_alive()
{
    unsigned long now = millis();
    int64_t timeSinceLastUpdate = now - lastUpdate;
    if (timeSinceLastUpdate >= PACK_ALIVE_TIMEOUT)
    {
        return false;
    }
    return true;
}

void BatteryPack::send_message(CANMessage *frame)
{
    /*
    printf("SEND :: id:%02X  [", frame->can_id);
    for ( int i = 0; i < frame->can_dlc; i++ ) {
        printf("%02X ", frame->data[i]);
    }
    printf("]\n");
    */
    ACAN_T4::BATTERY_CAN.tryToSend(*frame);
}

void BatteryPack::set_pack_error_status(int newErrorStatus)
{
    errorStatus = newErrorStatus;
}

int BatteryPack::get_pack_error_status()
{
    return errorStatus;
}

void BatteryPack::set_pack_balance_status(int newBalanceStatus)
{
    balanceStatus = newBalanceStatus;
}

int BatteryPack::get_pack_balance_status()
{
    return balanceStatus;
}

// Return true if it's time for the pack to be balanced.
bool BatteryPack::pack_is_due_to_be_balanced()
{
    return false;
    return ((millis() - nextBalanceTime) < 0);
}

void BatteryPack::reset_balance_timer()
{
    nextBalanceTime = millis() + BALANCE_INTERVAL/1000;
}

//// ----
//
// Voltage
//
//// ----

// Return the voltage of the whole pack
float BatteryPack::get_voltage()
{
    return voltage;
}

// Update the pack voltage value by summing all of the cell voltages
void BatteryPack::update_voltage()
{
    float newVoltage = 0;
    for (int m = 0; m < numModules; m++)
    {
        newVoltage += modules[m].get_voltage();
    }
    voltage = newVoltage;
}

// Return the voltage of the lowest cell in the pack
float BatteryPack::get_lowest_cell_voltage()
{
    float lowestCellVoltage = 10.0000f;
    for (int m = 0; m < numModules; m++)
    {
        // skip modules with incomplete cell data
        if (!modules[m].all_module_data_populated())
        {
            continue;
        }
        if (modules[m].get_lowest_cell_voltage() < lowestCellVoltage)
        {
            lowestCellVoltage = modules[m].get_lowest_cell_voltage();
        }
    }
    return lowestCellVoltage;
}

// Return true if any cell in the pack is under min voltage
bool BatteryPack::has_empty_cell()
{
    for (int m = 0; m < numModules; m++)
    {
        if (modules[m].has_empty_cell())
        {
            return true;
        }
    }
    return false;
}

// Return the voltage of the highest cell in the pack
float BatteryPack::get_highest_cell_voltage()
{
    float highestCellVoltage = 0.0000f;
    for (int m = 0; m < numModules; m++)
    {
        // skip modules with incomplete cell data
        if (!modules[m].all_module_data_populated())
        {
            continue;
        }
        if (modules[m].get_highest_cell_voltage() > highestCellVoltage)
        {
            highestCellVoltage = modules[m].get_highest_cell_voltage();
        }
    }
    return highestCellVoltage;
}

// Calculate largest voltage difference between cells in the pack and store result
void BatteryPack::update_cell_delta()
{
    cellDelta = (get_highest_cell_voltage() - get_lowest_cell_voltage()) * 1000.0;
}

// Return true if any cell in the pack is over max voltage
bool BatteryPack::has_full_cell()
{
    for (int m = 0; m < numModules; m++)
    {
        if (modules[m].has_full_cell())
        {
            return true;
        }
    }
    return false;
}

// Update the value for the voltage of an individual cell in a pack
void BatteryPack::update_cell_voltage(int moduleId, int cellIndex, float newCellVoltage)
{
    modules[moduleId].update_cell_voltage(cellIndex, newCellVoltage);
}

// Extract voltage readings from CAN message and updated stored values
void BatteryPack::decode_voltages(CANMessage *frame)
{

    // printf("    Inside decode_voltages\n");

    int messageId = (frame->id & 0x0F0);
    int moduleId = (frame->id & 0x00F);

    // printf("    decode voltages :: messageId : %02X, moduleId : %02X\n", messageId, moduleId);

    switch (messageId)
    {
    case 0x000:
        // error = msg.buf[0] + (msg.buf[1] << 8) + (msg.buf[2] << 16) + (msg.buf[3] << 24);
        set_pack_error_status(frame->data[0] + (frame->data[1] << 8) + (frame->data[2] << 16) + (frame->data[3] << 24));
        // balstat = (frame.data[5] << 8) + frame.data[4];
        set_pack_balance_status((frame->data[5] << 8) + frame->data[4]);
        break;
    case 0x020:
        modules[moduleId].update_cell_voltage(0, float(frame->data[0] + (frame->data[1] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(1, float(frame->data[2] + (frame->data[3] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(2, float(frame->data[4] + (frame->data[5] & 0x3F) * 256) / 1000);
        break;
    case 0x30:
        modules[moduleId].update_cell_voltage(3, float(frame->data[0] + (frame->data[1] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(4, float(frame->data[2] + (frame->data[3] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(5, float(frame->data[4] + (frame->data[5] & 0x3F) * 256) / 1000);
        break;
    case 0x40:
        modules[moduleId].update_cell_voltage(6, float(frame->data[0] + (frame->data[1] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(7, float(frame->data[2] + (frame->data[3] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(8, float(frame->data[4] + (frame->data[5] & 0x3F) * 256) / 1000);
        break;
    case 0x50:
        modules[moduleId].update_cell_voltage(9, float(frame->data[0] + (frame->data[1] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(10, float(frame->data[2] + (frame->data[3] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(11, float(frame->data[4] + (frame->data[5] & 0x3F) * 256) / 1000);
        break;
    case 0x60:
        modules[moduleId].update_cell_voltage(12, float(frame->data[0] + (frame->data[1] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(13, float(frame->data[2] + (frame->data[3] & 0x3F) * 256) / 1000);
        modules[moduleId].update_cell_voltage(14, float(frame->data[4] + (frame->data[5] & 0x3F) * 256) / 1000);
        break;
    case 0x70:
        modules[moduleId].update_cell_voltage(15, float(frame->data[0] + (frame->data[1] & 0x3F) * 256) / 1000);
        break;
    default:
        break;
    }

    // Check if this update has left us with a complete set of voltage/temperature readings
    if (!modules[moduleId].all_module_data_populated())
    {
        modules[moduleId].check_if_module_data_is_populated();
    }

    update_voltage();
    update_cell_delta();
}

//// ----
//
// Temperature
//
//// ----

// Return true if any cell in the pack is over max temperature
bool BatteryPack::has_temperature_sensor_over_max()
{
    for (int m = 0; m < numModules; m++)
    {
        if (modules[m].has_temperature_sensor_over_max())
        {
            return true;
        }
    }
    return false;
}

// Return the maximum current we can charge the pack with.
int BatteryPack::get_max_charging_current()
{
    int maxChargeCurrent = 0;
    for (int m = 0; m < numModules; m++)
    {
        if (modules[m].get_max_charging_current() < maxChargeCurrent)
        {
            maxChargeCurrent = modules[m].get_max_charging_current();
        }
    }
    return maxChargeCurrent;
}

// return the temperature of the lowest sensor in the pack
float BatteryPack::get_lowest_temperature()
{
    float lowestModuleTemperature = 1000;
    for (int m = 0; m < numModules; m++)
    {
        if (modules[m].get_lowest_temperature() < lowestModuleTemperature)
        {
            lowestModuleTemperature = modules[m].get_lowest_temperature();
        }
    }
    return lowestModuleTemperature;
}

// Extract temperature sensor readings from CAN frame and update stored values
void BatteryPack::decode_temperatures(CANMessage *temperatureMessageFrame)
{
    int moduleId = (temperatureMessageFrame->id & 0x00F) + 1;
    for (int t = 0; t < numTemperatureSensorsPerModule; t++)
    {
        modules[moduleId].update_temperature(t, temperatureMessageFrame->data[t] - 40);
    }
}
