#define MAIN_VCU
#ifdef MAIN_VCU
#ifndef COMMS_BMS_H
#define COMMS_BMS_H

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include <Watchdog_t4.h>

#include <settings.h>
#include <main/NissanPDM.h>

extern Scheduler scheduler;

// Commands to make the onboard LED blink
void led_blink();
void enable_led_blink();


//Commands to handle the PDM software module
extern NissanPDM charger;
void enable_PDM_monitor();
void enable_PDM_tasks();


#endif
#endif