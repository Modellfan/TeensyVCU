#ifndef SETTINGS_H
#define SETTINGS_H

#define VERSION 1.0

//---------------------------------------------------------------------------------------------------------------------------------------------
//CMU and pack settings
//---------------------------------------------------------------------------------------------------------------------------------------------
#define NUM_PACKS         1
#define CELLS_PER_MODULE 12                        // The number of cells in each module
#define TEMPS_PER_MODULE  4                        // The number of temperature sensors in each module
#define MODULES_PER_PACK  8                        // The number of modules in each pack

//CMU Settings
#define CMU_MAX_INTERNAL_WARNING_TEMPERATURE 70.00f
#define CMU_MAX_DELTA_MODULE_CELL_VOLTAGE 2.00f
#define CMU_MIN_PLAUSIBLE_VOLTAGE 2.00f
#define CMU_MAX_PLAUSIBLE_VOLTAGE 5.00f
#define CMU_MAX_PLAUSIBLE_TEMPERATURE 80.00f
#define CMU_MIN_PLAUSIBLE_TEMPERATURE -40.00f                    

//Pack Setting
#define BATTERY_CAN can2
#define PACK_WAIT_FOR_NUM_MODULES 1
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
#define CONTACTOR_TIMEOUT 200
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

#define BMS_TOTAL_CAPACITY 345 * 3600 * CELLS_PER_MODULE; //Num modules missing 345Wh 3600 s/H 12 Cells. Unit: Ws

//BMW i3 Specs from Samsung SDI document
#define BMS_SOC_WARNING_LIMIT 0.8 //Below this limit, max current values are not allowed anymore

#define BMS_MAX_DISCHARGE_PEAK_CURRENT 406 * 0.9 //Safetyfactor

// //Discharge Limits
// const int16_t temperatures[] PROGMEM = {60, 50, 40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -40};
// const int16_t continuous_discharge_CurrentLimits[] PROGMEM = {223, 223, 223, 210, 196, 180, 166, 153, 136, 124, 108, 93, 77, 74, 62, 57, 46, 33};
// #define BMS_DISCHARGE_TIME_INTEGRAL 150

// //Charge Limits
// const int16_t temperatures_charge[] PROGMEM = {60, 50, 40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -40};
// const int16_t chargeCurrentLimits_peak[] PROGMEM = {270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 237, 185, 125, 62, 33, 22, 7, 1};
// const int16_t Irms_charge[] PROGMEM = {107, 107, 96, 84, 73, 61, 51, 41, 32, 24, 18, 12, 7.2, 4.3, 2.7, 1.7, 1.0, 0.4};
// #define BMS_CHARGE_TIME_INTEGRAL 100

// #define SAFETY_LIMIT_CHARGE 4.25
// #define SAFETY_LIMIT_DISCHARGE 1.5

// #define OPERATING_LIMIT_CHARGE 4.15
// #define OPERATING_LIMIT_DISCHARGE 2.7

// #define SAFETY_LIMIT_MAX_STORAGE_TEMP 80
// #define SAFETY_LIMIT_MIN_STORAGE_TEMP -40

// #define SAFETY_LIMIT_MAX_OPERATION_TEMP 80
// #define SAFETY_LIMIT_MIN_OPERATION_TEMP -40

// #define OPERATION_LIMIT_MAX_TEMP 60
// #define OPERATION_LIMIT_MIN_TEMP -40

// //SOC LUT
// const int16_t soc[] PROGMEM = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0};
// float temperatureLevels[4] = {40, 25, -10, -25};

// float dischargeTable[11][4] = {
//     {4.129, 4.129, 4.131, 4.132},
//     {4.009, 4.010, 4.012, 4.013},
//     {3.9076, 3.907, 3.909, 3.910},
//     {3.818, 3.819, 3.820, 3.821},
//     {3.750, 3.751, 3.751, 3.752},
//     {3.677, 3.676, 3.671, 3.669},
//     {3.641, 3.641, 3.634, 3.647},
//     {3.616, 3.614, 3.611, 3.611},
//     {3.572, 3.574, 3.579, 3.581},
//     {3.452, 3.490, 3.496, 3.499},
//     {3.395, 3.404, 3.422, 3.429}
// };


// const int16_t soc[] PROGMEM = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0};
// const float chargeCurrentLimits_25C[] PROGMEM = {4.133, 4.022, 3.921, 3.833, 3.765, 3.690, 3.653, 3.627, 3.598, 3.517, 3.433};

// //416A

// // Define the data types for x1s, x2s, and ys
// typedef int X1Type; // SOC
// typedef float X2Type; // Temperature
// typedef float YType; // Resistance (mΩ)

// // Define the data table
// const X1Type x1sData[11] = {100, 95, 90, 80, 70, 60, 50, 40, 30, 20, 10, 5};
// const X2Type x2sData[4] = {40, 25, -10, -25};
// const YType ysData[11][4] = {
//     {0.53, 0.69, 2.22, 3.85},
//     {0.53, 0.68, 2.22, 3.84},
//     {0.52, 0.68, 2.22, 3.87},
//     {0.52, 0.68, 2.25, 3.90},
//     {0.52, 0.68, 2.29, 4.00},
//     {0.52, 0.68, 2.34, 4.14},
//     {0.52, 0.68, 2.40, 4.32},
//     {0.51, 0.68, 2.84, 5.66},
//     {0.52, 0.68, 3.59, 8.41},
//     {0.54, 0.74, 5.20, 14.68},
//     {0.60, 0.99, 9.53, 18.01},
//     {0.71, 1.81, 11.30, 16.31}
// };

// //294A
// // Define the table dimensions
// const int numRows = 12; // Number of rows including the header
// const int numCols = 5;  // Number of columns including the SOC column

// // Define the data types for x1s, x2s, and ys
// typedef int X1Type; // SOC
// typedef float X2Type; // Temperature
// typedef float YType; // Resistance (mΩ)

// // Define the data table
// const X1Type x1sData[numRows] = {100, 95, 90, 80, 70, 60, 50, 40, 30, 20, 10, 5};
// const X2Type x2sData[numCols] = {40, 25, -10, -25};
// const YType ysData[numRows - 1][numCols - 1] = {
//     {0.75, 0.96, 2.77, 4.29},
//     {0.74, 0.95, 2.77, 4.31},
//     {0.73, 0.94, 2.76, 4.39},
//     {0.72, 0.93, 2.77, 4.42},
//     {0.71, 0.91, 2.81, 4.54},
//     {0.70, 0.91, 2.89, 4.71},
//     {0.67, 0.90, 2.98, 5.09},
//     {0.65, 0.88, 3.24, 5.44},
//     {0.67, 0.91, 3.72, 10.69},
//     {0.75, 1.03, 6.47, 19.61},
//     {0.90, 1.88, 14.63, 39.36},
//     {1.51, 3.79, 22.52, 48.00}
// };



//Archive

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

#define WATCHDOG_TIMEOUT 150 // Set the watchdog timeout in milliseconds



// Debug function for teleplot app in vscode
#define PLOT(name, value) \
    do { \
        SerialUSB1.print(">" #name ":"); \
        SerialUSB1.println(value); \
    } while(0)

#endif