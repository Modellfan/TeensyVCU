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
#include "can_packer.h"
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
        modules[m] = BatteryModule(m, this);
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

    balanceActive = false;
    balanceTargetVoltage = 4.27;

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

    // Byte 0 & 1: hold the target voltage for balancing
    if (balanceActive == true)
    {
        pollModuleFrame.data[0] = lowByte((uint16_t(balanceTargetVoltage * 1000) + 0));
        pollModuleFrame.data[1] = highByte((uint16_t(balanceTargetVoltage * 1000) + 0));
    }
    else
    {
        pollModuleFrame.data[0] = 0xC7;
        pollModuleFrame.data[1] = 0x10;
    }

    // Byte 2: is always 0x00
    pollModuleFrame.data[2] = 0x00;

    // Byte 3: For the first three messages 0x00. Then 0x50 to signalize that temperature and voltage should be measured.
    // Byte 4: For the first three messages 0x00. Then 0x08 if balancing active or 0x00 if balancing is inactive
    if (inStartup)
    {
        pollModuleFrame.data[3] = 0x00;
        pollModuleFrame.data[4] = 0x00;
    }
    else
    {
        pollModuleFrame.data[3] = 0x50; // 0x00 request no measurements, 0x50 request voltage and temp, 0x10 request voltage measurement, 0x40 request temperature measurement.//balancing bits
        if (balanceActive == 1)
        {
            pollModuleFrame.data[4] = 0x08; // 0x00 request no balancing
        }
        else
        {
            pollModuleFrame.data[4] = 0x00;
        }
    }

    // Byte 5: is alsways 0x00
    pollModuleFrame.data[5] = 0x00;

    // Byte 6: has the frame counter in the higher nibble. One bit is set at the end of startup.
    pollModuleFrame.data[6] = modulePollingCycle << 4;
    if (inStartup && (modulePollingCycle == 2))
    {
        pollModuleFrame.data[6] = pollModuleFrame.data[6] + 0x04;
    }

    // Byte 7: is the checksum
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

    CANMessage msg;

    if (ACAN_T4::BATTERY_CAN.receive(msg))
    {
        for (int i = 0; i < numModules; i++)
        {
            modules[i].process_message(msg);
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

void BatteryPack::set_balancing_active(bool status)
{
    balanceActive = status;
}

void BatteryPack::set_balancing_voltage(float voltage)
{
    balanceTargetVoltage = voltage;
}

// Return the voltage of the lowest cell in the pack
float BatteryPack::get_lowest_cell_voltage()
{
    float lowestCellVoltage = 10.0000f;
    // for (int m = 0; m < numModules; m++)
    // {
    //     // skip modules with incomplete cell data
    //     if (!modules[m].all_module_data_populated())
    //     {
    //         continue;
    //     }
    //     if (modules[m].get_lowest_cell_voltage() < lowestCellVoltage)
    //     {
    //         lowestCellVoltage = modules[m].get_lowest_cell_voltage();
    //     }
    // }
    return lowestCellVoltage;
}

// Return the voltage of the highest cell in the pack
float BatteryPack::get_highest_cell_voltage()
{
    float highestCellVoltage = 0.0000f;
    // for (int m = 0; m < numModules; m++)
    // {
    //     // skip modules with incomplete cell data
    //     if (!modules[m].all_module_data_populated())
    //     {
    //         continue;
    //     }
    //     if (modules[m].get_highest_cell_voltage() > highestCellVoltage)
    //     {
    //         highestCellVoltage = modules[m].get_highest_cell_voltage();
    //     }
    // }
    return highestCellVoltage;
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
