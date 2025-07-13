#include "settings.h"
#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include <Watchdog_t4.h>

#include "comms_bms.h"

#include <bms/current.h>
#include <bms/contactor.h>
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "bms/battery i3/pack.h"
#include "bms/battery_manager.h"

//---------------------------------------------------------------------------------------------------------------------------------------------
//IPace ISA Shunt Software Component
//---------------------------------------------------------------------------------------------------------------------------------------------
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

//---------------------------------------------------------------------------------------------------------------------------------------------
//Contactor Manager Software Component
//---------------------------------------------------------------------------------------------------------------------------------------------
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
}

//---------------------------------------------------------------------------------------------------------------------------------------------
// BMW i3 Battery Software Component
//---------------------------------------------------------------------------------------------------------------------------------------------
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

Task poll_battery_for_data_timer(13, TASK_FOREVER, &poll_battery_for_data); // Original BMW i3 polls each pack at 100ms at 8 modules 13 ms comes to approx 100ms

void enable_poll_battery_for_data()
{
    scheduler.addTask(poll_battery_for_data_timer);
    poll_battery_for_data_timer.enable();
    Serial.println("Battery poll timer enabled.");
}

//---------------------------------------------------------------------------------------------------------------------------------------------
// Main Battery Manager Software Component
//---------------------------------------------------------------------------------------------------------------------------------------------
void BMS_Task2ms()
{
    battery_manager.Task2Ms();
}

void BMS_Task10ms()
{
    //battery_manager.Task10Ms();
}

void BMS_Task100ms()
{
    //battery_manager.Task100Ms();
}

Task BMS_task2ms_timer(2, TASK_FOREVER, &BMS_Task2ms);
Task BMS_Task10ms_timer(10, TASK_FOREVER, &BMS_Task10ms);
Task BMS_task100ms_timer(100, TASK_FOREVER, &BMS_Task100ms);

void enable_BMS_tasks()
{
    scheduler.addTask(BMS_task2ms_timer);
    BMS_task2ms_timer.enable();
    scheduler.addTask(BMS_Task10ms_timer);
    BMS_Task10ms_timer.enable();
    scheduler.addTask(BMS_task100ms_timer);
    BMS_task100ms_timer.enable();
    Serial.println("BMS handle tasks timer enabled.");
}

void BMS_Monitor100ms()
{
    //battery_manager.Monitor100Ms();
}

void BMS_Monitor1000ms()
{
    //battery_manager.Monitor1000Ms();
}

Task BMS_monitor_100ms_timer(100, TASK_FOREVER, &BMS_Monitor100ms);
Task BMS_monitor_1000ms_timer(1000, TASK_FOREVER, &BMS_Monitor1000ms);

void enable_BMS_monitor()
{
    battery_manager.initialize();
    scheduler.addTask(BMS_monitor_100ms_timer);
    BMS_monitor_100ms_timer.enable();
    scheduler.addTask(BMS_monitor_1000ms_timer);
    BMS_monitor_1000ms_timer.enable();
    Serial.println("BMS monitor enabled.");
}

//---------------------------------------------------------------------------------------------------------------------------------------------
// Monitoring & Debug printing
//---------------------------------------------------------------------------------------------------------------------------------------------
void print_debug()
{
  
}

Task print_debug_timer(1000, TASK_FOREVER, &print_debug);

void enable_print_debug()
{
    scheduler.addTask(print_debug_timer);
    print_debug_timer.enable();
    Serial.println("Print debug timer enabled.");
}

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

//---------------------------------------------------------------------------------------------------------------------------------------------
//CPU Load Monitoring
//---------------------------------------------------------------------------------------------------------------------------------------------
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

