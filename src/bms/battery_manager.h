
#ifndef BMS_H
#define BMS_H

#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery i3/pack.h"
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "utils/can_packer.h"
#include "settings.h"

typedef void (*SendMessageCallback)(const CANMessage &);

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
        DTC_BMS_NONE = 0,
        DTC_BMS_CAN_SEND_ERROR = 1 << 0,
        DTC_BMS_CAN_INIT_ERROR = 1 << 1,
        DTC_BMS_PACK_FAULT = 1 << 2,
    } DTC_BMS;

    enum VehicleState
    {
        STATE_SLEEP = 0,
        STATE_STANDBY,
        STATE_READY,
        STATE_CONDITIONING,
        STATE_DRIVE,
        STATE_CHARGE,
        STATE_ERROR,
        STATE_LIMP_HOME
    };

    static const uint16_t VCU_STATUS_MSG_ID = 0x437; // CAN message containing vehicle state

    BMS(BatteryPack &_batteryPack, Shunt_ISA_iPace &_shunt, Contactormanager &_contactorManager); // Constructor taking a reference to BatteryPack

    // Runnables
    //         void print();
    void initialize();
    void read_message(); // Poll messages

    void Task2Ms();  // Read Can messages ?
    void Task10Ms(); // Poll messages
    void Task100Ms();
    void Task1000Ms();

private:
    BatteryPack &batteryPack; // Reference to the BatteryPack
    Shunt_ISA_iPace &shunt;
    Contactormanager &contactorManager;

    bool cell_available[CELLS_PER_MODULE * MODULES_PER_PACK];
    float internal_resistance[CELLS_PER_MODULE * MODULES_PER_PACK];
    float open_circuite_voltage[CELLS_PER_MODULE * MODULES_PER_PACK];

    float power; // in Ws

    // SOC
    float soc_coulomb_counting;

    // Non-Volatile Variable!!
    float watt_seconds;
    float ampere_seconds;
    float total_capacity = BMS_TOTAL_CAPACITY;

    void send_battery_status_message();

    //         // State maschine updating
    //         void update_state_machine();

    // Helper functions
    void send_message(CANMessage *frame); // Send out CAN message

    byte moduleToBeMonitored;

    // Current vehicle state received from the VCU
    VehicleState vehicle_state;

    // State and DTC
    STATE_BMS state;
    DTC_BMS dtc;

    float max_charge_current;
    float max_discharge_current;
    bool ready_to_shutdown;
    //         const char *getStateString();
    //         String getDTCString();
};

#endif
