#include "settings.h"
#ifdef BMS_VCU
#ifndef COMMS_BMS_H
#define COMMS_BMS_H

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include <Watchdog_t4.h>

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
#endif