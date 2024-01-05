#ifndef PACK_H
#define PACK_H

#include <ACAN_T4.h>
#include <Arduino.h>

#include "module.h"
#include "utils/can_packer.h"
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
    bool get_cell_voltage(byte cellIndex, float &voltage);

    // Temperature
    float get_lowest_temperature();
    float get_highest_temperature();
    bool get_cell_temperature(byte cell, float &temperature);

private:
    BatteryPack &batteryPack;  // Reference to the BatteryPack

    // Private functions
    void calculate_current_limits();
    void calculate_soc();
    void calculate_soc_lut();
    void calculate_soc_ekf();
    void calculate_soc_coulomb_counting();
    void calculate_open_circuit_voltage();
    void update_state_machine();

    CANMessage outFrame;

    //sate and dtc
    STATE_PACK state;
    DTC_PACK dtc;

    const char *getStateString();
    String getDTCString();
};

#endif
