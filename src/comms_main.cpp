#include "settings.h"
#ifdef MAIN_VCU
#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include <Watchdog_t4.h>

#include "comms_main.h"
#include <main/NissanPDM.h>


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

//---------------------------------------------------------------------------------------------------------------------------------------------
// NissanPDM software module runnables
//---------------------------------------------------------------------------------------------------------------------------------------------
void PDM_Task2ms()
{
    charger.Task2Ms();
}

void PDM_Task10ms()
{
    charger.Task10Ms();
}

void PDM_Task100ms()
{
    charger.Task100Ms();
}

Task PDM_task2ms_timer(2, TASK_FOREVER, &PDM_Task2ms);
Task PDM_Task10ms_timer(10, TASK_FOREVER, &PDM_Task10ms);
Task PDM_task100ms_timer(100, TASK_FOREVER, &PDM_Task100ms);

void enable_PDM_tasks()
{
    scheduler.addTask(PDM_task2ms_timer);
    PDM_task2ms_timer.enable();
    scheduler.addTask(PDM_Task10ms_timer);
    PDM_Task10ms_timer.enable();
    scheduler.addTask(PDM_task100ms_timer);
    PDM_task100ms_timer.enable();
    Serial.println("PDM handle tasks timer enabled.");
}

void PDM_Monitor100ms()
{
    charger.Monitor100Ms();
}

void PDM_Monitor1000ms()
{
    charger.Monitor1000Ms();
}

Task PDM_monitor_100ms_timer(100, TASK_FOREVER, &PDM_Monitor100ms);
Task PDM_monitor_1000ms_timer(1000, TASK_FOREVER, &PDM_Monitor1000ms);

void enable_PDM_monitor()
{
    charger.initialize();
    scheduler.addTask(PDM_monitor_100ms_timer);
    PDM_monitor_100ms_timer.enable();
    scheduler.addTask(PDM_monitor_1000ms_timer);
    PDM_monitor_1000ms_timer.enable();
    Serial.println("PDM monitor enabled.");
}
#endif

