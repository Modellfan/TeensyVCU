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

    // Our state
    STATE_CMU getState();
    DTC_CMU getDTC();

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
    float get_temperature(byte tempindex);
    float get_internal_temperature();

    //Balancing
    bool get_is_balancing();
    bool get_balancing(byte cell);

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
};

#endif
