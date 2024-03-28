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
#include <Watchdog_t4.h>

#include <settings.h>

#include <bms/current.h>
#include <bms/contactor.h>
#include "bms/contactor_manager.h"
#include "bms/battery i3/pack.h"
#include "bms/battery_manager.h"

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

//Commands to handle the BMS software module
extern BMS battery_manager;
void handle_bms_CAN_messages();
void enable_handle_bms_CAN_messages();
void BMS_monitor_100ms();
void BMS_Monitor1000ms();
void enable_BMS_monitor();
void enable_BMS_tasks();

//Debug
extern int balancecount;
void print_debug();
void enable_print_debug();

#endif