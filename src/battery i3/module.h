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

#include "settings.h"

class BatteryPack;

class BatteryModule {

    private:
        int id;
        int numCells;                            // Number of cells in this module
        int numTemperatureSensors;               // Number of temperature sensors in this module

        float cellVoltage[CELLS_PER_MODULE];     // Voltages of each cell
        float cellTemperature[TEMPS_PER_MODULE]; // Temperatures of each cell
        u_int8_t cellBalancing[CELLS_PER_MODULE];
        float totalVoltage;

        u_int16_t balanceStatus; 
        u_int64_t errorStatus;

        bool allModuleDataPopulated;             // True when we have voltage/temp information for all cells
        u_int32_t lastUpdate;                    // Times out with setting value of PACK_ALIVE_TIMEOUT in ms
        
        BatteryPack* pack;                       // The parent BatteryPack that contains this module

    public:
        BatteryModule();
        BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors);

        bool all_module_data_populated();
        void check_if_module_data_is_populated();
        bool module_is_alive();

        void print();

        //Misc signals from csc
        void update_error_status();
        void update_balancing_status();

        // Voltage
        float get_voltage();
        float get_lowest_cell_voltage();
        float get_highest_cell_voltage();
        void update_cell_voltage(int cellIndex, float newCellVoltage);
        void update_module_voltage(float newModuleVoltage);
        void update_cell_balancing(int cellIndex, u_int8_t newCellBalancing);
        bool has_empty_cell();
        bool has_full_cell();

        // Temperature
        void update_temperature(int tempSensorId, float newTemperature);
        void update_csc_temperature(float newTemperature);
        float get_lowest_temperature();
        float get_highest_temperature();
        bool has_temperature_sensor_over_max();
        bool temperature_at_warning_level();
};

#endif
