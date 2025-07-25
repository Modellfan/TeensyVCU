#ifndef SETTINGS_H
#define SETTINGS_H

#define VERSION 2.0
//#define DEBUG

//---------------------------------------------------------------------------------------------------------------------------------------------
//CMU and pack settings
//---------------------------------------------------------------------------------------------------------------------------------------------
#define CELLS_PER_MODULE 12                        // The number of cells in each module
#define TEMPS_PER_MODULE  4                        // The number of temperature sensors in each module
#define MODULES_PER_PACK  8                        // The number of possible  modules in each pack

//CMU Settings
#define CMU_MAX_INTERNAL_WARNING_TEMPERATURE 70.00f
#define CMU_MAX_DELTA_MODULE_CELL_VOLTAGE 2.00f
#define CMU_MIN_PLAUSIBLE_VOLTAGE 2.00f
#define CMU_MAX_PLAUSIBLE_VOLTAGE 5.00f
#define CMU_MAX_PLAUSIBLE_TEMPERATURE 80.00f
#define CMU_MIN_PLAUSIBLE_TEMPERATURE -40.00f                    

//Pack Setting
#define BATTERY_CAN can2
#define PACK_WAIT_FOR_NUM_MODULES 7
#define PACK_ALIVE_TIMEOUT 300 

//---------------------------------------------------------------------------------------------------------------------------------------------
// Shunt Settings
//---------------------------------------------------------------------------------------------------------------------------------------------
#define ISA_SHUNT_CAN can1
#define ISA_SHUNT_TIMEOUT 100 //Timeout in ms, when no new message is recieved 
#define ISA_SHUNT_MAX_TEMPERATURE 70

//---------------------------------------------------------------------------------------------------------------------------------------------
// Contactor Manager Settings
//---------------------------------------------------------------------------------------------------------------------------------------------
#define CONTACTOR_TIMELOOP 20
#define CONTACTOR_TIMEOUT 500
#define CONTACTOR_DEBOUNCE 100 //Debounce should be bigger than time loop

#define CONTACTOR_CLOSED_STATE LOW

#define CONTACTOR_POWER_SUPPLY_IN_PIN 6

#define CONTACTOR_NEG_IN_PIN 28
#define CONTACTOR_POS_IN_PIN 8
#define CONTACTOR_POS_OUT_PIN 4
#define CONTACTOR_PRCHG_IN_PIN 5
#define CONTACTOR_PRCHG_OUT_PIN 3

#define CONTACTOR_PRCHG_TIME 2000 //Time for the precharge contactor to be closed before positive contactor

//---------------------------------------------------------------------------------------------------------------------------------------------
//BMS software module settings
//---------------------------------------------------------------------------------------------------------------------------------------------
#define BMS_CAN can3

#define BMS_VCU_MSG_ID 0x437
#define BMS_MSG_VOLTAGE 0x41A
#define BMS_MSG_CELL_TEMP 0x41B
#define BMS_MSG_LIMITS 0x41C
#define BMS_MSG_SOC 0x41D
#define BMS_MSG_HMI 0x41E
#define BMS_VCU_TIMEOUT 300

#define BMS_TOTAL_CAPACITY 345 * 3600 * CELLS_PER_MODULE; //Num modules missing 345Wh 3600 s/H 12 Cells. Unit: Ws

//BMW i3 Specs from Samsung SDI document
#define SAFETY_LIMIT_CHARGE 4.25
#define SAFETY_LIMIT_DISCHARGE 1.5

#define OPERATING_LIMIT_CHARGE 4.15
#define OPERATING_LIMIT_DISCHARGE 2.7

#define SAFETY_LIMIT_MAX_STORAGE_TEMP 80
#define SAFETY_LIMIT_MIN_STORAGE_TEMP -40

#define SAFETY_LIMIT_MAX_OPERATION_TEMP 80
#define SAFETY_LIMIT_MIN_OPERATION_TEMP -40

#define OPERATION_LIMIT_MAX_TEMP 60
#define OPERATION_LIMIT_MIN_TEMP -40

// Default current limit used in limp home mode when the BMS detects a fault
#define BMS_LIMP_HOME_DISCHARGE_CURRENT 50.0f
#define BMS_LIMP_HOME_CHARGE_CURRENT    0.0f

// Internal resistance online estimation parameters
#define IR_ESTIMATION_CURRENT_STEP_THRESHOLD 1.0f
#define IR_ESTIMATION_ALPHA 0.1f

// Voltage thresholds for current derating
#define V_MIN_DERATE 3.5f //based on SOC
#define V_MIN_CUTOFF OPERATING_LIMIT_DISCHARGE
#define V_MAX_DERATE 4.0f //based on SOC
#define V_MAX_CUTOFF OPERATING_LIMIT_CHARGE

// Maximum absolute pack current (A) for valid OCV-based SOC estimation
#define BMS_OCV_CURRENT_THRESHOLD 10.0f

// Maximum allowable instantaneous pack discharge current (A)
#define BMS_MAX_DISCHARGE_PEAK_CURRENT 409.0f

// -----------------------------------------------------------------------------
// Balancing Settings
// -----------------------------------------------------------------------------
#define BALANCE_DELTA_V      0.015f   // Minimum delta between cells to start balancing (V)
#define BALANCE_MIN_VOLTAGE  3.4f     // Do not balance below this cell voltage (V)
#define BALANCE_MAX_TEMP     50.0f    // Do not balance above this temperature (°C)
#define BALANCE_OFFSET_V     0.005f   // Offset added to lowest cell voltage for target (V)







//Archive

// Battery

// // Official min pack voltage = 269V. 269 / 6 / 16 = 2.8020833333V
// #define CELL_EMPTY_VOLTAGE 2.9

// // Official max pack voltage = 398V. 398 / 6 / 16 = 4.1458333333V
// #define CELL_FULL_VOLTAGE 4.0

// #define CELL_UNDER_TEMPERATURE_FAULT_THRESHOLD 0    // degrees
// #define CELL_OVER_TEMPERATURE_WARNING_THRESHOLD 55  // degrees
// #define CELL_OVER_TEMPERATURE_FAULT_THRESHOLD 65    // degrees

// #define CHARGE_THROTTLE_TEMP_LOW  45                // Start throttling charge current when battery sensors above this temperature
// #define CHARGE_THROTTLE_TEMP_HIGH 65                // Top of the throttling scale. Limit current to CHARGE_CURRENT_MIN at and above this temperature
// #define CHARGE_CURRENT_MAX 125                      // ~50kw
// #define CHARGE_CURRENT_MIN 8                        // ~3.3kw

// #define BALANCE_INTERVAL 1200                       // number of seconds between balancing sessions

// #define WATCHDOG_TIMEOUT 150 // Set the watchdog timeout in milliseconds


#endif