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

#ifndef COMMS_H
#define COMMS_H

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include <settings.h>

#include <current.h>
#include <contactor.h>
#include "contactor_manager.h"
#include "battery i3/pack.h"

extern Scheduler scheduler;

// Commands to make the onboard LED blink
void led_blink();
void enable_led_blink();

// Commands to handle the current sensor
extern Shunt_ISA_iPace shunt;
void update_shunt();
void enable_update_shunt();

// Commands to handle the contactors
extern Contactormanager contactor_manager;
void update_contactors();
void enable_update_contactors();

void update_system_load();
void enable_update_system_load();

// Command to handle the BMW i3 Battery
extern BatteryPack batteryPack;
void handle_battery_CAN_messages();
void enable_handle_battery_CAN_messages();
void poll_battery_for_data();
void enable_poll_battery_for_data();


//Debug
void print_debug();
void enable_print_debug();


// void enable_status_print();
// void request_module_data(BatteryModule *module);
// bool poll_all_modules_for_data(struct repeating_timer *t);
// void enable_module_polling();
// bool send_status_message();
// void enable_status_messages();
// bool send_charge_limits_message();
// void enable_charge_limits_messages();
// void disable_charge_limits_messages();

// bool handle_main_CAN_messages(struct repeating_timer *t);
// void enable_handle_main_CAN_messages();
// bool handle_battery_CAN_messages(struct repeating_timer *t);
// void enable_handle_battery_CAN_messages();

#endif