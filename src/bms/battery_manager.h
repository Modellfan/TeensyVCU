       
#ifndef BMS_H
#define BMS_H

#include <ACAN_T4.h>
#include <Arduino.h>

#include "pack.h"
#include "utils/can_packer.h"
#include "settings.h"

class BMS
{

public:
    enum STATE_BMS
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
    } DTC_BMS;

    BMS();
    BatteryPack(int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule);

    // Runnables
    void print();
    void initialize();

    void Task2Ms(); //Send out message
    void Task10Ms(); //Poll messages
    void Task100Ms();

    void Monitor100Ms();



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
    int numModules;                     //
    int numCellsPerModule;              //
    int numTemperatureSensorsPerModule; //
    
    BatteryModule modules[MODULES_PER_PACK]; // The child modules that make up this BatteryPack



    //sate and dtc
    STATE_BMS state;
    DTC_BMS dtc;

    const char *getStateString();
    String getDTCString();
};

#endif
   
        // Charging
        int get_max_charging_current();


        //Thermo management

        //Balancing control

        //SOC estimation


        //Event and error handling

        //Derating


        //Input Value plausibilization with error healing

        // Constructor
BMS::BMS(BatteryPack &_batteryPack) : batteryPack(_batteryPack)
{
    // Initialize other members if needed
}

// Private function definitions
void BMS::calculate_current_limits()
{
    // Implementation
}

void BMS::calculate_soc()
{
    // Implementation
}

void BMS::calculate_soc_lut()
{
    // Implementation
}

void BMS::calculate_soc_ekf()
{
    // Implementation
}

void BMS::calculate_soc_coulomb_counting()
{
    // Implementation
}

void BMS::calculate_open_circuit_voltage()
{
    // Implementation
}

void BMS::update_state_machine()
{
    // Implementation
}

// Public function definitions
void BMS::runnable_update()
{
    // Implementation
}

void BMS::runnable_send_CAN_message()
{
    // Implementation
}