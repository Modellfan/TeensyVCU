// SERIAL-LINK-RECEIVER-FROM-BATTERY.h

#ifndef SERIAL_LINK_RECEIVER_FROM_BATTERY_H
#define SERIAL_LINK_RECEIVER_FROM_BATTERY_H

#define BATTERY_SELECTED

#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for all defines
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

//  https://github.com/mackelec/SerialDataLink

// These parameters need to be mapped on the battery side
extern uint16_t system_max_design_voltage_dV;  //V+1,  0-500.0 (0-5000)
extern uint16_t system_min_design_voltage_dV;  //V+1,  0-500.0 (0-5000)
extern uint16_t system_real_SOC_pptt;          //SOC%, 0-100.00 (0-10000)
extern uint16_t system_SOH_pptt;               //SOH%, 0-100.00 (0-10000)
extern uint16_t system_battery_voltage_dV;     //V+1,  0-500.0 (0-5000)
extern int16_t system_battery_current_dA;      //A+1, -1000 - 1000
extern uint32_t system_capacity_Wh;            //Wh,  0-150000Wh
extern uint32_t system_remaining_capacity_Wh;  //Wh,  0-150000Wh
extern uint16_t system_max_discharge_power_W;  //W,    0-65000
extern uint16_t system_max_charge_power_W;     //W,    0-65000
extern uint8_t system_bms_status;              //Enum 0-5
extern int16_t system_active_power_W;          //W, -32000 to 32000
extern int16_t system_temperature_min_dC;      //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;      //C+1, -50.0 - 50.0
extern uint16_t system_cell_max_voltage_mV;    //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;    //mV, 0-5000, Stores the minimum cell millivolt value
extern bool system_LFP_Chemistry;              //Set to true or false depending on cell chemistry
extern bool batteryAllowsContactorClosing;     //Bool, 1=true, 0=false

// Parameters to send to the transmitter
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false

void manageSerialLinkReceiver();
void update_values_serial_link();
void setup_battery(void);

#endif
