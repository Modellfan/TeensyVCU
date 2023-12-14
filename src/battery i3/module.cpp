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

BatteryModule::BatteryModule(){}

BatteryModule::BatteryModule (int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors) {

    //printf("Creating module (id:%d, pack:%d, cpm:%d, t:%d)\n", _id, _pack->id, _numCells, _numTemperatureSensors);

    id = _id;

    // Point back to parent pack
    pack = _pack;

    // Initialise all cell voltages to zero
    numCells = _numCells;


    for ( int c = 0; c < numCells; c++ ) {
        cellVoltage[c] = 0.000f;
        cellBalancing[c] = 0;
    }

    // Initialise temperature sensor readings to zero
    numTemperatureSensors = _numTemperatureSensors;

    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        cellTemperature[t] = -50.000f;
    }

    allModuleDataPopulated = false;

    //printf("    module %d creation complete\n", id);

}

void BatteryModule::print () {
    Serial.printf("    Module id : %d (populated : %d; alive : %d)\n", id, all_module_data_populated(), module_is_alive());
    Serial.printf("        Cell Voltages : ");
    for ( int c = 0; c < numCells; c++ ) {
        Serial.printf("%d:%1.3fV B%d ", c, cellVoltage[c], cellBalancing[c]);
    }
    Serial.printf("\n");
    Serial.printf("        Temperatures : ");
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        Serial.printf("%d:%3.2fC ", t, cellTemperature[t]);
    }
    Serial.printf("\n");
}

// Return total module voltage by summing the cell voltages
float BatteryModule::get_voltage() {
    float voltage = 0;
    for ( int c = 0; c < numCells; c++ ) {
        voltage += cellVoltage[c];
    }
    return voltage;
}

// Return the voltage of the lowest cell voltage in the module
float BatteryModule::get_lowest_cell_voltage() {
    float lowestCellVoltage = 10.0000f;
    for ( int c = 0; c < numCells; c++ ) {
        //printf("Comparing %3.3f and %3.3f\n", cellVoltage[c], lowestCellVoltage);
        if ( cellVoltage[c] < lowestCellVoltage ) {
            lowestCellVoltage = cellVoltage[c];
        }
    }
    return lowestCellVoltage;
}

// Return the voltage of the highest cell in the module
float BatteryModule::get_highest_cell_voltage() {
    float highestCellVoltage = 0.000f;
    for ( int c = 0; c < numCells; c++ ) {
        //printf("module : comparing : %.4f to %.4f\n", cellVoltage[c], highestCellVoltage);
        if ( cellVoltage[c] > highestCellVoltage ) {
            highestCellVoltage = cellVoltage[c];
        }
    }
    return highestCellVoltage;
}

// Update the voltage for a single cell
void BatteryModule::update_cell_voltage(int cellIndex, float newCellVoltage) {
    //printf("module : update_cell_voltage : %d : %.4f\n", cellIndex, newCellVoltage);
    cellVoltage[cellIndex] = newCellVoltage;
    lastUpdate = millis();
}

void BatteryModule::update_cell_balancing(int cellIndex, u_int8_t newCellBalancing) {
    cellBalancing[cellIndex] = newCellBalancing;
}

// Return true if we have voltage/temp information for all cells
bool BatteryModule::all_module_data_populated() {
    return allModuleDataPopulated;
}

void BatteryModule::check_if_module_data_is_populated() {
    bool voltageMissing = false;
    for ( int c = 0; c < numCells; c++ ) {
        if ( cellVoltage[c] == 0.000f ) {
            voltageMissing = true;
        }
    }
    bool temperatureMissing = false;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        if ( cellTemperature[t] == -50.000f ) {
            temperatureMissing = true;
        }
    }
    allModuleDataPopulated = voltageMissing && temperatureMissing;
}

// Update the value for one of the temperature sensors
void BatteryModule::update_temperature(int tempSensorId, float newTemperature) {
    cellTemperature[tempSensorId] = newTemperature;
    lastUpdate = millis();
}

bool BatteryModule::module_is_alive()
{
    int64_t timeSinceLastUpdate = millis() - lastUpdate;
    if (timeSinceLastUpdate >= PACK_ALIVE_TIMEOUT)
    {
        return false;
    }
    return true;
}


// Return the temperature of the coldest sensor in the module
float BatteryModule::get_lowest_temperature() {
    float lowestTemperature = 1000.0f;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        if ( cellTemperature[t] < lowestTemperature ) {
            lowestTemperature = cellTemperature[t];
        }
    }
    return lowestTemperature;
}

// Return the temperature of the hottest sensor in the module
float BatteryModule::get_highest_temperature() {
    float highestTemperature = -50;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        if ( cellTemperature[t] > highestTemperature ) {
            highestTemperature = cellTemperature[t];
        }
    }
    return highestTemperature;
}




