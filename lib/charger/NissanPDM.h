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
    enum STATE_PACK
    {
        INIT,      // pack is being initialized
        OPERATING, // pack is in operation state
        FAULT      // pack is in fault state
    };

    typedef enum
    {
        DTC_PACK_NONE = 0,
        DTC_PACK_CAN_SEND_ERROR = 1 << 0,
        DTC_PACK_CAN_INIT_ERROR = 1 << 1,
        DTC_PACK_MODULE_FAULT = 1 << 2,
    } DTC_PACK;

    BatteryPack();
    BatteryPack(int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule);

    // Runnables
    void print();
    void initialize();

    void request_data(); //Send out message
    void read_message(); //Poll messages

    // Helper functions
    void send_message(CANMessage *frame);      // Send out CAN message
    uint8_t getcheck(CANMessage &msg, int id); // Calculate BMW i3 checksum

    // Setter and getter
    void set_balancing_active(bool status);
    void set_balancing_voltage(float voltage);
    bool get_any_module_balancing();

    // Voltage
    float get_lowest_cell_voltage();
    float get_highest_cell_voltage();
    float get_pack_voltage();
    float get_delta_cell_voltage();
    bool get_cell_voltage(byte cell, float voltage);

    // Temperature
    float get_lowest_temperature();
    float get_highest_temperature();
    bool get_cell_temperature(byte cell, float temperature);

private:
    int numModules;                     //
    int numCellsPerModule;              //
    int numTemperatureSensorsPerModule; //
    
    BatteryModule modules[MODULES_PER_PACK]; // The child modules that make up this BatteryPack

    float balanceTargetVoltage;
    bool balanceActive;

    //private variables for polling
    uint8_t pollMessageId;
    bool initialised;
    CRC8 crc8;
    bool inStartup;
    uint8_t modulePollingCycle;
    uint8_t moduleToPoll;
    CANMessage pollModuleFrame;

    //sate and dtc
    STATE_PACK state;
    DTC_PACK dtc;

    const char *getStateString();
    String getDTCString();
};

#endif


/*
 * This file is part of the ZombieVeter project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *               2021-2022 Damien Maguire <info@evbmw.com>
 * Yes I'm really writing software now........run.....run away.......
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
#ifndef NISSANPDM_H
#define NISSANPDM_H

#include <stdint.h>
#include "my_fp.h"
#include "chargerhw.h"

class NissanPDM: public Chargerhw
{
public:
   void DecodeCAN(int id, uint32_t data[2]);
   void Task10Ms();
   void Task100Ms();
   bool ControlCharge(bool RunCh, bool ACReq);
//   void SetTorque(float torque);
//   float GetMotorTemperature() { return motor_temp; }
//   float GetInverterTemperature() { return inv_temp; }
//   float GetInverterVoltage() { return voltage / 2; }
//   float GetMotorSpeed() { return speed / 2; }
//   int GetInverterState() { return error; }
   void SetCanInterface(CanHardware* c);

private:
   static void nissan_crc(uint8_t *data, uint8_t polynomial);
   static int8_t fahrenheit_to_celsius(uint16_t fahrenheit);
//   uint32_t lastRecv;
//   int16_t speed;
//   int16_t inv_temp;
//   int16_t motor_temp;
//   bool error;
//   uint16_t voltage;
//   int16_t final_torque_request;
};

#endif // NISSANPDM_H



