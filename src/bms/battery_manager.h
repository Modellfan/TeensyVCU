
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

    // --- Inputs / Raw Data ---
    float cell_voltage[CELLS_PER_MODULE * MODULES_PER_PACK];
    float cell_temp[CELLS_PER_MODULE * MODULES_PER_PACK];
    float pack_current;
    float dt;
    bool invalid_data;

    float power; // in Ws

    // SOC
    float soc_ocv_lut;       // SOC from OCV-Temp LUT
    float soc_coulomb_counting; // SOC from coulomb counting
    float soc;               // Corrected SOC

    // --- SOH (Monitoring Only) ---
    float measured_capacity_Ah; // Integrated capacity over a full cycle

    // --- Current Limits (Temperature) ---
    float current_limit_peak_discharge;        // Peak allowed current (A) - discharge direction
    float current_limit_rms_discharge;         // RMS allowed current (A)  - discharge direction
    float current_limit_peak_charge; // Peak charge current (A)
    float current_limit_rms_charge;  // Continuous charge current (A)
    float current_limit_rms_derated_discharge; // Derated RMS current limit for discharge
    float current_limit_rms_derated_charge;    // Derated RMS current limit for charge

    // --- Internal Resistance (IR) ---
    float internal_resistance_table;                                // IR from LUT
    float internal_resistance_estimated;                            // IR from online estimation
    float internal_resistance_estimated_cells[CELLS_PER_MODULE * MODULES_PER_PACK]; // IR from online estimation, per cell
    float internal_resistance_used;                                 // Max(IR_table, IR_estimated)
    float internal_resistance_used_min;                             // Smallest IR (best cell)
    float internal_resistance_used_max;                             // Largest IR (worst cell)

    // --- Voltage Derating ---
    // Placeholder for derating factor/state

    // --- Dynamic Voltage Limit ---
    float current_limit_voltage_dynamic;

    // --- RMS Calculation ---
    // Placeholder for EMA and soft clamp state variables

    // --- Final Allowed Current ---
    float current_limit_allowed;     // Output of min-selector
    float current_limit_limp_home;   // Limp home current limit
    float current_limit_selected;    // After limp home logic
    float current_limit_final;       // After rate limiter

    // Non-Volatile Variable!!
    float watt_seconds;
    float ampere_seconds;
    float total_capacity = BMS_TOTAL_CAPACITY;

    void send_battery_status_message();

    // --- Core Functions ---
    void update_soc_ocv_lut();
    void update_soc_coulomb_counting();
    void correct_soc();
    void calculate_soh();

    void lookup_current_limits();
    void lookup_internal_resistance_table();
    void estimate_internal_resistance_online();
    void select_internal_resistance_used();

    void calculate_voltage_derate();
    void calculate_rms_ema();
    void calculate_dynamic_voltage_limit();

    void select_current_limit();

    void calculate_limp_home_limit();
    void select_limp_home();

    void rate_limit_current();

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
