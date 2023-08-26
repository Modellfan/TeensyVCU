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

    // printf("Initialising BatteryPack %d\n", 1);

    numModules = _numModules;
    numCellsPerModule = _numCellsPerModule;
    numTemperatureSensorsPerModule = _numTemperatureSensorsPerModule;

    // Initialise modules
    for (int m = 0; m < numModules; m++)
    {
        // printf("Creating module %d (cpm:%d, tpm:%d)\n", m, numCellsPerModule, numTemperatureSensorsPerModule);
        modules[m] = BatteryModule(m, this, numCellsPerModule, numTemperatureSensorsPerModule);
    }
}

void BatteryPack::initialize()
{
    // Set up CAN port
    ACAN_T4_Settings settings(500 * 1000); // 125 kbit/s
    const uint32_t errorCode = ACAN_T4::BATTERY_CAN.begin(settings);

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
        Serial.println("Battery CAN ok");
    }
    else
    {
        Serial.print("Error Battery CAN: 0x");
        Serial.println(errorCode, HEX);
    }

    voltage = 0.0000f;
    cellDelta = 0;

    inStartup = true;
    modulePollingCycle = 0;
    moduleToPoll = 0;

    crc8.begin();
}

void BatteryPack::print()
{
    Serial.printf("Pack %d : %3.2fV : %dmV\n", 1, voltage, cellDelta);
    Serial.println("");
    for (int m = 0; m < numModules; m++)
    {
        modules[m].print();
    }
}

// Calculate the checksum for a message to be sent out.
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
    // modulePollinCycle is the frameCounter send to the BMS
    if (modulePollingCycle == 0xF)
    {
        modulePollingCycle = 0;
    }

    // numModules counts from 0 to 7 for a BMW i3
    if (moduleToPoll == numModules)
    {
        moduleToPoll = 0;
        modulePollingCycle++;
    }

    pollModuleFrame.id = 0x080 | (moduleToPoll);
    pollModuleFrame.len = 8;
    pollModuleFrame.data[0] = 0xC7;
    pollModuleFrame.data[1] = 0x10;
    pollModuleFrame.data[2] = 0x00;
    pollModuleFrame.data[3] = 0x50;
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
    pollModuleFrame.data[7] = getcheck(pollModuleFrame, moduleToPoll);
    
    send_message(&pollModuleFrame);

    // Only run this one, for the last module being polled
    if (inStartup && (modulePollingCycle == 2) && (moduleToPoll == (numModules - 1)))
    {
        inStartup = false;
    }

    
    moduleToPoll++;
    return;
}

void BatteryPack::read_message()
{

    // Serial.printf("Pack %d : checking for messages\n", 1);

    CANMessage frame;

    if (ACAN_T4::BATTERY_CAN.receive(frame))
    {

        // Serial.printf("Pack %d received message : id:%02X : ", 1, frame.id);
        // for (int i = 0; i < frame.len; i++)
        // {
        //     Serial.printf("%02X ", frame.data[i]);
        // }
        // Serial.printf("\n");

        // Temperature messages
        if ((frame.id & 0xFF0) == 0x170)
        {
            decode_temperatures(&frame);
        }

        // Voltage messages
        if (frame.id > 0x99 && frame.id < 0x160)
        {
            decode_voltages(&frame);
        }
    }
}

void BatteryPack::send_message(CANMessage *frame)
{

    // Serial.printf("SEND :: id:%02X  [", frame->id);
    // for ( int i = 0; i < frame->len; i++ ) {
    //     Serial.printf("%02X ", frame->data[i]);
    // }
    // Serial.printf("]\n");

    if (ACAN_T4::BATTERY_CAN.tryToSend(*frame))
    {
        // Serial.println("Send ok");
    }
}

void BatteryPack::set_pack_error_status(int newErrorStatus)
{
    errorStatus = newErrorStatus;
    //getError() & 0x2000 >= 0)
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

// Update the value for the voltage of an individual cell in a pack
void BatteryPack::update_cell_voltage(int moduleId, int cellIndex, float newCellVoltage)
{
    modules[moduleId].update_cell_voltage(cellIndex, newCellVoltage);
}

// Extract voltage readings from CAN message and updated stored values
void BatteryPack::decode_voltages(CANMessage *frame)
{

    // Serial.printf("    Inside decode_voltages\n");

    int messageId = (frame->id & 0x0F0);
    int moduleId = (frame->id & 0x00F);

    // Serial.printf("    decode voltages :: messageId : %02X, moduleId : %02X\n", messageId, moduleId);

    switch (messageId)
    {
    case 0x000:
        set_pack_error_status(frame->data[0] + (frame->data[1] << 8) + (frame->data[2] << 16) + (frame->data[3] << 24));
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
    default:
        break;
    }

    // Check if this update has left us with a complete set of voltage/temperature readings
    if (!modules[moduleId].all_module_data_populated())
    {
        modules[moduleId].check_if_module_data_is_populated();
    }

    update_voltage();
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
    int moduleId = (temperatureMessageFrame->id & 0x00F);
    for (int t = 0; t < numTemperatureSensorsPerModule; t++)
    {
        modules[moduleId].update_temperature(t, temperatureMessageFrame->data[t] - 40);
    }

    // Check if this update has left us with a complete set of voltage/temperature readings
    if (!modules[moduleId].all_module_data_populated())
    {
        modules[moduleId].check_if_module_data_is_populated();
    }
}
