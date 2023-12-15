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

    void request_data();
    void read_message();

    // Helper functions
    void send_message(CANMessage *frame);      // Send out CAN message
    uint8_t getcheck(CANMessage &msg, int id); // Calculate BMW i3 checksum

    // Setter and getter
    void set_balancing_active(bool status);
    void set_balancing_voltage(float voltage);

    // Voltage
    float get_lowest_cell_voltage();
    float get_highest_cell_voltage();

    // Temperature
    float get_lowest_temperature();
    float get_highest_temperature();

private:
    int numModules;                     //
    int numCellsPerModule;              //
    int numTemperatureSensorsPerModule; //
    float voltage;                      // Voltage of the total pack
    int cellDelta;                      // Difference in voltage between high and low cell, in mV

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
