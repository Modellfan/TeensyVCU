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

#ifndef MODULE_H
#define MODULE_H

#include <ACAN_T4.h>
#include <Arduino.h>
#include "settings.h"

class BatteryPack;

class BatteryModule
{

public:
    enum STATE_CMU
    {
        INIT,      // CMU is being initialized
        OPERATING, // CMU is in operation state
        FAULT      // CMU is in fault state
    };

    typedef enum
    {
        DTC_CMU_NONE = 0,
        DTC_CMU_INTERNAL_ERROR = 1 << 0,
        DTC_CMU_TEMPERATURE_TOO_HIGH = 1 << 1,
        DTC_CMU_SINGLE_VOLTAGE_IMPLAUSIBLE = 1 << 2,
        DTC_CMU_TEMPERATURE_IMPLAUSIBLE = 1 << 3,
        DTC_CMU_TIMED_OUT = 1 << 4,
        DTC_CMU_MODULE_VOLTAGE_IMPLAUSIBLE = 1 << 5
    } DTC_CMU;

    BatteryModule();
    BatteryModule(int _id, BatteryPack *_pack);

    // Runnables
    void process_message(CANMessage &msg);
    void check_alive();
    void print();

    // Our state
    STATE_CMU getState();

    // Our Signals
    //  Voltage
    float get_voltage();
    float get_lowest_cell_voltage();
    float get_highest_cell_voltage();
    float get_cell_voltage(byte c);

    // Temperature
    float get_lowest_temperature();
    float get_highest_temperature();
    float get_average_temperature();


    //Balancing
    bool get_is_balancing();

private:
    int id;
    int numCells;              // Number of cells in this module
    int numTemperatureSensors; // Number of temperature sensors in this module

    float cellVoltage[CELLS_PER_MODULE];     // Voltages of each cell
    float cellTemperature[TEMPS_PER_MODULE]; // Temperatures of each cell
    bool cellBalance[CELLS_PER_MODULE];      //
    bool balanceDirection[CELLS_PER_MODULE];
    float moduleVoltage;
    float temperatureInternal;
    bool cmuError;

    STATE_CMU state;
    DTC_CMU dtc;

    u_int32_t lastUpdate; // Times out with setting value of PACK_ALIVE_TIMEOUT in ms
    BatteryPack *pack;    // The parent BatteryPack that contains this module

    const char *getStateString();
    String getDTCString();

    bool check_if_module_data_is_populated();
    bool plausibilityCheck();

    void printFrame(CANMessage &frame);
};

#endif
