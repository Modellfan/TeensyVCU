
#ifndef BMS_H
#define BMS_H

#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery i3/pack.h"
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "utils/can_packer.h"
#include "settings.h"
#include "persistent_data_storage.h"

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
        DTC_BMS_CONTACTOR_FAULT = 1 << 3,
        DTC_BMS_SHUNT_FAULT = 1 << 4,
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

    BMS(BatteryPack &_batteryPack, Shunt_ISA_iPace &_shunt, Contactormanager &_contactorManager); // Constructor taking a reference to BatteryPack

    // Runnables
    //         void print();
    void initialize();
    void read_message(); // Poll messages

    void set_pack_power(float pack_power_w);

    void Task2Ms();  // Read Can messages ?
    void Task10Ms(); // Poll messages
    void Task100Ms();
    void Task1000Ms();

    // Balancing control
    void update_balancing();

    // Accessors for monitoring values
    STATE_BMS get_state() const { return state; }
    DTC_BMS get_dtc() const { return dtc; }
    VehicleState get_vehicle_state() const { return vehicle_state; }
    bool get_ready_to_shutdown() const { return ready_to_shutdown; }
    bool get_vcu_timeout() const { return vcu_timeout; }
    bool is_vcu_data_valid() const { return !vcu_timeout; }
    float get_max_charge_current() const { return max_charge_current; }
    float get_max_discharge_current() const { return max_discharge_current; }
    float get_soc() const { return soc; }
    float get_soc_ocv_lut() const { return soc_ocv_lut; }
    float get_soc_coulomb_counting() const { return soc_coulomb_counting; }
    float get_current_limit_peak_discharge() const { return current_limit_peak_discharge; }
    float get_current_limit_rms_discharge() const { return current_limit_rms_discharge; }
    float get_current_limit_peak_charge() const { return current_limit_peak_charge; }
    float get_current_limit_rms_charge() const { return current_limit_rms_charge; }
    float get_current_limit_rms_derated_discharge() const { return current_limit_rms_derated_discharge; }
    float get_current_limit_rms_derated_charge() const { return current_limit_rms_derated_charge; }

    bool is_balancing_finished() const { return balancing_finished; }

private:
    PersistentDataStorage persistent_storage;

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

    // SOC
    float soc_ocv_lut;          // SOC from OCV-Temp LUT
    uint32_t ocv_current_settle_start_ms;
    float soc_coulomb_counting; // SOC from coulomb counting
    float soc;                  // Corrected SOC

    // --- Current Limits (Temperature) ---
    float current_limit_peak_discharge;        // Peak allowed discharge current (A)
    float current_limit_rms_discharge;         // RMS allowed discharge current (A)
    float current_limit_peak_charge;           // Peak charge current (A)
    float current_limit_rms_charge;            // Continuous charge current (A)
    float current_limit_rms_derated_discharge; // Derated RMS current limit for discharge
    float current_limit_rms_derated_charge;    // Derated RMS current limit for charge

    // --- Internal Resistance (IR) ---
    float internal_resistance_table;                                                // IR from LUT
    float internal_resistance_estimated;                                            // IR from online estimation
    float internal_resistance_estimated_cells[CELLS_PER_MODULE * MODULES_PER_PACK]; // IR from online estimation, per cell
    float internal_resistance_used;                                                 // Max(IR_table, IR_estimated)
    float internal_resistance_used_min;                                             // Smallest IR (best cell)
    float internal_resistance_used_max;                                             // Largest IR (worst cell)

    // --- Voltage Derating ---
    // Placeholder for derating factor/state

    // --- Dynamic Voltage Limit ---
    float current_limit_voltage_dynamic;

    // --- RMS Calculation ---
    // Placeholder for EMA and soft clamp state variables

    // --- Final Allowed Current ---
    float current_limit_allowed;   // Output of min-selector
    float current_limit_limp_home; // Limp home current limit
    float current_limit_selected;  // After limp home logic
    float current_limit_final;     // After rate limiter

    // Balancing finished flag
    bool balancing_finished;

    // HMI Energy Metrics
    float avg_energy_per_hour;        // kWh per hour
    float time_remaining_s;           // Remaining time until empty/full depending on sign
    float avg_power_w;                // Internal average power for filtering
    float last_instantaneous_power_w; // Latest instantaneous power sample (W)
    float remaining_wh; // Remaining energy in Wh

    // Non-Volatile Variable!!  
    float energy_initial_Wh;
    float measured_capacity_Wh = BMS_INITIAL_REMAINING_WH;
    float ampere_seconds_initial;
    float measured_capacity_Ah;

    void send_battery_status_message();

    // --- Core Functions ---
    void update_soc_ocv_lut();
    void update_soc_coulomb_counting();
    void correct_soc();
    void calculate_soh();
    void update_energy_metrics();

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
    void update_state_machine();

    // Helper functions
    void send_message(CANMessage *frame); // Send out CAN message

    byte moduleToBeMonitored;

    uint8_t msg1_counter;
    uint8_t msg2_counter;
    uint8_t msg3_counter;
    uint8_t msg4_counter;
    uint8_t msg5_counter;

    uint8_t vcu_counter;
    unsigned long last_vcu_msg;
    bool vcu_timeout;

    // Current vehicle state received from the VCU
    VehicleState vehicle_state;
    VehicleState last_vehicle_state;

    void apply_persistent_data(const PersistentDataStorage::PersistentData &data);
    PersistentDataStorage::PersistentData collect_persistent_data() const;

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
