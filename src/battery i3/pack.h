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

#ifndef PACK_H
#define PACK_H

#include <ACAN_T4.h>
#include <Arduino.h>

#include "module.h"
#include "can_packer.h"
#include "settings.h"
#include "CRC8.h"

class BatteryModule;

const uint8_t finalxor[12] = {0xCF, 0xF5, 0xBB, 0x81, 0x27, 0x1D, 0x53, 0x69, 0x02, 0x38, 0x76, 0x4C};

class BatteryPack
{

public:
    BatteryPack();
    BatteryPack(int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule);

    void print();
    void initialize();
    
    void request_data(); 
    void read_message();

    void send_message(CANMessage *frame); //Send out CAN message
    uint8_t getcheck(CANMessage &msg, int id); //Calculate BMW i3 checksum

    void set_balancing_active(bool status);
    void set_balancing_voltage(float voltage);

    // Voltage
    float get_lowest_cell_voltage();
    float get_highest_cell_voltage();

    // Temperature
    float get_lowest_temperature();
 

private:
    int numModules;                     //
    int numCellsPerModule;              //
    int numTemperatureSensorsPerModule; //
    float voltage;                      // Voltage of the total pack
    int cellDelta;                      // Difference in voltage between high and low cell, in mV

    float balanceTargetVoltage;
    bool balanceActive;
    
    uint8_t pollMessageId;           //
    bool initialised;
    BatteryModule modules[MODULES_PER_PACK]; // The child modules that make up this BatteryPack
    CRC8 crc8;

    bool inStartup;
    uint8_t modulePollingCycle;
    uint8_t moduleToPoll;
    CANMessage pollModuleFrame;
};

#endif
