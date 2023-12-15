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

#ifndef SETTINGS_H
#define SETTINGS_H

#define VERSION 1.0

// Serial port
#define UART_ID      uart0
#define BAUD_RATE   115200
#define UART_TX_PIN      0 // pin 1
#define UART_RX_PIN      1 // pin 2


//#define NUM_PACKS         2                        // The total number of paralleled packs in this battery
#define NUM_PACKS         1
#define CELLS_PER_MODULE 12                        // The number of cells in each module
#define TEMPS_PER_MODULE  4                        // The number of temperature sensors in each module
#define MODULES_PER_PACK  8                        // The number of modules in each pack

//CMU Settings
#define CMU_MAX_INTERNAL_WARNING_TEMPERATURE 50.00f
#define CMU_MAX_DELTA_MODULE_CELL_VOLTAGE 2.00f
#define CMU_MIN_PLAUSIBLE_VOLTAGE 2.00f
#define CMU_MAX_PLAUSIBLE_VOLTAGE 5.00f
#define CMU_MAX_PLAUSIBLE_TEMPERATURE 80.00f
#define CMU_MIN_PLAUSIBLE_TEMPERATURE -40.00f
#define PACK_ALIVE_TIMEOUT 300                       // If we have not seen an update from the BMS in
                                                   // PACK_ALIVE_TIMEOUT seconds, then mark the pack
                                                   // as dead.
//Pack Setting
#define PACK_WAIT_FOR_NUM_MODULES 1

#define SAFE_VOLTAGE_DELTA_BETWEEN_PACKS 0.01      // When closing contactors, the voltage difference between the packs shall not
                                                   // be greater than this voltage, in volts.

//const int CS_PINS[2] = { 20, 15 };               // Chip select pins for the CAN controllers for each battery pack.
//const int CS_PINS[2] = { 15, 15 };
const int CS_PINS[1] = { 15, };

// Outputs
#define CHARGE_INHIBIT_PIN 4                       // Low-side switch to create CHARGE_INHIBIT signal. a.k.a OUT1
#define HEATER_ENABLE_PIN 5                        // Low-side switch to turn on battery heaters. a.k.a. OUT2
const int INHIBIT_CONTACTOR_PINS[2] = { 2, 3 };    // Low-side switch to disallow closing of battery box contactors
#define DRIVE_INHIBIT_PIN 6

// Inputs
#define IGNITION_ENABLE_PIN 10
#define CHARGE_ENABLE_PIN 9
#define IN_1_PIN 11
#define IN_2_PIN 12
#define IN_3_PIN 13
#define IN_4_PIN 14

#define SPI_PORT      spi0

#define SPI_MISO        16 // pin 21
#define SPI_CLK         18 // pin 24
#define SPI_MOSI        19 // pin 25

// mainNet
#define MAIN_CAN_CS     17 // pin 22

#define CAN_CLK_PIN     21 // pin 27

// Battery

// Official min pack voltage = 269V. 269 / 6 / 16 = 2.8020833333V
#define CELL_EMPTY_VOLTAGE 2.9

// Official max pack voltage = 398V. 398 / 6 / 16 = 4.1458333333V
#define CELL_FULL_VOLTAGE 4.0

#define CELL_UNDER_TEMPERATURE_FAULT_THRESHOLD 0    // degrees
#define CELL_OVER_TEMPERATURE_WARNING_THRESHOLD 55  // degrees
#define CELL_OVER_TEMPERATURE_FAULT_THRESHOLD 65    // degrees

#define CHARGE_THROTTLE_TEMP_LOW  45                // Start throttling charge current when battery sensors above this temperature
#define CHARGE_THROTTLE_TEMP_HIGH 65                // Top of the throttling scale. Limit current to CHARGE_CURRENT_MIN at and above this temperature
#define CHARGE_CURRENT_MAX 125                      // ~50kw
#define CHARGE_CURRENT_MIN 8                        // ~3.3kw

#define BALANCE_INTERVAL 1200                       // number of seconds between balancing sessions

#define BATTERY_CAN can2

// Shunt Settings
#define ISA_SHUNT_CAN can1
#define ISA_SHUNT_TIMEOUT 100 //Timeout in ms, when no new message is recieved 

// Contactor Manager Settings
#define CONTACTOR_TIMELOOP 20
#define CONTACTOR_TIMEOUT 200
#define CONTACTOR_DEBOUNCE 100 //Debounce should be bigger than time loop

#define CONTACTOR_NEG_IN_PIN 3
#define CONTACTOR_NEG_OUT_PIN 2
#define CONTACTOR_POS_IN_PIN 5
#define CONTACTOR_POS_OUT_PIN 4
#define CONTACTOR_PRCHG_IN_PIN 29
#define CONTACTOR_PRCHG_OUT_PIN 28

#define CONTACTOR_PRCHG_TIME 2000 //Time for the precharge contactor to be closed before positive contactor


// Debug function for teleplot app in vscode
#define PLOT(name, value) \
    do { \
        SerialUSB1.print(">" #name ":"); \
        SerialUSB1.println(value); \
    } while(0)

#endif