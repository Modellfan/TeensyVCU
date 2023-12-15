/*
 * This file is part of TeensyVCU project
 *
 * Copyright (C) 2023 Felix Schuster
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#include "settings.h"
#include "comms.h"

#include <current.h>
#include <contactor.h>
#include "contactor_manager.h"
#include "battery i3/pack.h"

// #include "statemachine.h"
// #include "battery.h"
// #include "pack.h"

// struct repeating_timer statusPrintTimer;

// bool status_print(struct repeating_timer *t) {
//     extern Battery battery;
//     battery.print();
//     return true;
// }

// void enable_status_print() {
//     add_repeating_timer_ms(5000, status_print, NULL, &statusPrintTimer);
// }

// //// ----
// //
// // Polling the modules for voltage and temperature readings
// //
// //// ----

// struct can_frame pollModuleFrame;

// struct repeating_timer pollModuleTimer;

// // Send request to each pack to ask for a data update
// bool poll_packs_for_data(struct repeating_timer *t) {
//     extern Battery battery;
//     battery.request_data();
//     return true;
// }

// void enable_module_polling() {
//     add_repeating_timer_ms(1000, poll_packs_for_data, NULL, &pollModuleTimer);
// }

// Blink the built in led to visualize main state
void led_blink()
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    // Serial.println("Serial Alive");
}

Task led_blink_timer(1000, TASK_FOREVER, &led_blink);

void enable_led_blink()
{
    scheduler.addTask(led_blink_timer);
    led_blink_timer.enable();
    Serial.println("Led blink enabled.");
}

// Poll the Jaguar iPace Shunt
void update_shunt()
{
    shunt.update();
}

Task update_shunt_timer(5, TASK_FOREVER, &update_shunt);

void enable_update_shunt()
{
    shunt.initialise();
    scheduler.addTask(update_shunt_timer);
    update_shunt_timer.enable();
    Serial.println("Shunt update timer enabled.");
}

// Monitor CPU load
void update_system_load()
{
    // Serial.print("Monitor");
}

Task update_system_load_timer(100, TASK_FOREVER, &update_system_load);

void enable_update_system_load()
{
    scheduler.addTask(update_system_load_timer);
    update_system_load_timer.enable();
    Serial.println("System Load update timer enabled.");
}

// Reset Watchdog

// Update the contactor manager
void update_contactors()
{
    contactor_manager.update();
}

Task update_contactors_timer(CONTACTOR_TIMELOOP, TASK_FOREVER, &update_contactors);

void enable_update_contactors()
{
    contactor_manager.initialise();
    scheduler.addTask(update_contactors_timer);
    update_contactors_timer.enable();
    Serial.println("Contactors update timer enabled.");
    contactor_manager.close();
}

// //// ----
// //
// // status messages
// //
// //// ----

// // CAN frame to hold status message sent out to rest of car
// // bit 0-4 = state (0=standby, 1=drive, 2=charging, 3=overTempFault, 4=underVoltageFault )
// struct can_frame statusFrame;

// struct repeating_timer statusMessageTimer;

// bool send_status_message(struct repeating_timer *t) {
//     extern State state;
//     extern MCP2515 mainCAN;
//     statusFrame.can_id = STATUS_MSG_ID;
//     statusFrame.can_dlc = 1;
//     if ( state == state_standby ) {
//         statusFrame.data[0] = 0x00 << 4;
//     } else if ( state == state_drive ) {
//         statusFrame.data[0] = 0x01 << 4;
//     } else if ( state == state_charging ) {
//         statusFrame.data[0] = 0x02 << 4;
//     } else if ( state == state_batteryEmpty ) {
//         statusFrame.data[0] = 0x03 << 4;
//     } else if ( state == state_fault ) {
//         statusFrame.data[0] = 0x04 << 4;
//     } else {
//         //
//     }
//     // do pack dead check

//     mainCAN.sendMessage(&statusFrame);
//     return true;
// }

// void enable_status_messages() {
//     add_repeating_timer_ms(1000, send_status_message, NULL, &statusMessageTimer);
// }

// //// ----
// //
// // send charge limits message (id = 0x102)
// //
// //// ----

// // CAN frame to hold charge limits message
// // byte 0 = 0x0
// // byte 1 = DC voltage limit MSB
// // byte 2 = DC voltage limit LSB
// // byte 3 = DC current set point
// // byte 4 = 1 == enable charging
// // byte 5 = SoC
// // byte 6 = 0x0
// // byte 7 = 0x0
// // From https://openinverter.org/wiki/Tesla_Model_S/X_GEN2_Charger
// struct can_frame chargeLimitsFrame;

// struct repeating_timer chargeLimitsMessageTimer;

// bool send_charge_limits_message(struct repeating_timer *t) {

//     extern Battery battery;

//     chargeLimitsFrame.can_id = 0x102;
//     chargeLimitsFrame.can_dlc = 8;

//     // byte 0
//     chargeLimitsFrame.data[0] = 0x0;
//     // byte 1 -- DC voltage limit MSB
//     chargeLimitsFrame.data[1] = 0x0; //fixme
//     // byte 2 -- DC voltage limit LSB
//     chargeLimitsFrame.data[2] = 0x0; //fixme
//     // byte 3 -- DC current set point
//     chargeLimitsFrame.data[3] = (__u8)battery.get_max_charging_current();
//     // byte 4 -- 1 == enable charging
//     chargeLimitsFrame.data[4] = 0x0; //fixme
//     // byte 5 -- SoC
//     chargeLimitsFrame.data[5] = 0x0; //fixme
//     // byte 6 -- 0x0
//     chargeLimitsFrame.data[6] = 0x0;
//     // byte 7 -- 0x0
//     chargeLimitsFrame.data[7] = 0x0;

//     return true;
// }

// void enable_charge_limits_messages() {
//     add_repeating_timer_ms(1000, send_charge_limits_message, NULL, &chargeLimitsMessageTimer);
// }

// void disable_charge_limits_messages() {
//       //
// }

// //// ----
// //
// // Inbound message handlers
// //
// //// ----

// struct can_frame mainCANInbound;
// struct repeating_timer handleMainCANMessageTimer;

// bool handle_main_CAN_messages(struct repeating_timer *t) {

//     extern MCP2515 mainCAN;

//     if ( mainCAN.readMessage(&mainCANInbound) == MCP2515::ERROR_OK ) {
//         if ( mainCANInbound.can_id == CAN_ID_ISA_SHUNT_WH ) {
//             // process Wh data
//         } else if ( mainCANInbound.can_id == CAN_ID_ISA_SHUNT_AH ) {
//             // process Ah data
//         }
//     }

//     return true;
// }

// void enable_handle_main_CAN_messages() {
//     add_repeating_timer_ms(10, handle_main_CAN_messages, NULL, &handleMainCANMessageTimer);
// }

void handle_battery_CAN_messages()
{
    extern BatteryPack batteryPack;
    batteryPack.read_message();
}

Task handle_battery_CAN_messages_timer(2, TASK_FOREVER, &handle_battery_CAN_messages);

void enable_handle_battery_CAN_messages()
{
    batteryPack.initialize();
    scheduler.addTask(handle_battery_CAN_messages_timer);
    handle_battery_CAN_messages_timer.enable();
    Serial.println("Battery handle CAN messages timer enabled.");
}

void poll_battery_for_data()
{
    extern BatteryPack batteryPack;
    batteryPack.request_data();
}

Task poll_battery_for_data_timer(13, TASK_FOREVER, &poll_battery_for_data); // Orginal BMW i3 polls each pack at 100ms at 8 modules 13 ms comes to approx 100ms

void enable_poll_battery_for_data()
{
    scheduler.addTask(poll_battery_for_data_timer);
    poll_battery_for_data_timer.enable();
    Serial.println("Battery poll timer enabled.");
}

// Print debug messages
void print_debug()
{
    extern BatteryPack batteryPack;
    batteryPack.print();
    balancecount++;
    if (balancecount == 10)
    {
        batteryPack.set_balancing_voltage(3.9);
        batteryPack.set_balancing_active(true);
    }
}

Task print_debug_timer(1000, TASK_FOREVER, &print_debug);

void enable_print_debug()
{
    scheduler.addTask(print_debug_timer);
    print_debug_timer.enable();
    Serial.println("Print debug timer enabled.");
}


// // Update the battery manager
// void update_battery_manager()
// {
//     extern BatteryPack batteryPack;
//     extern BatteryManager batteryManager;

//     //Set signals from battery
//     batteryManager.setMinCellVoltage(float newVoltage);
//     batteryManager.setMaxCellVoltage(float newVoltage);
//     batteryManager.setPackVoltage(float newVoltage);

//     batteryManager.setMinTemperature(float newtemperature);
//     batteryManager.setMaxTemperature(float newtemperature);

//     batteryManager.setPackState();
    
//     //Set signals from current sensor
//     batteryManager.setCurrentIntegral(float current_integral); // in mAh since last request
//     batteryManager.setCurrentDerivative(); //mA/s change of current since last update
//     batteryManager.setCurrent();
//     batteryManager.setCurrentState();

//     //Set signals from contactor manager
//     batteryManager.setContactorManagerState();


//     //Set signals from digital inputs

//     batteryManager.update();

//     //Get signal for contactor manager
//     contactor_manager.close();

//     //Get signal for battery 
//     batteryPack.set_balancing_voltage
//     batteryPack.set_balancing_active

// }

// Task update_battery_manager_timer(200, TASK_FOREVER, &update_battery_manager);

// void enable_update_battery_manager()
// {
//     scheduler.addTask(update_battery_manager_timer);
//     update_battery_manager_timer.enable();
//     Serial.println("Update contactor manager timer enabled.");
// }


//Comms for UDS messages

//Comms for error handling?

//Comms for sending out Nissan leaf messages