#include <Arduino.h>
#include <ACAN_T4.h>

#include <TaskScheduler.h>
#include "utils/Map2D3D.h"
#include <Watchdog_t4.h>
#include "settings.h"
#include <bms/current.h>
#include <bms/contactor.h>
#include "bms/contactor_manager.h"
#include "bms/battery i3/pack.h"
#include "bms/battery_manager.h"
#include "comms_bms.h"
#include "serial_console.h"

#ifndef __IMXRT1062__
#error "This sketch should be compiled for Teensy 4.1"
#endif

// Create system objects
Scheduler scheduler;
WDT_T4<WDT3> wdt; // use the RTWDT which should be the safest one

void wdtCallback()
{
  Serial.println("Watchdog was not fed. It will eat you soon. Sorry...");
  // The callback for WDOG3 is also different from the other 2 watchdogs, it is not called earlier like a trigger,
  // it's fired 255 cycles before the actual reset, so you have little time to collect diagnostic info before the reset happens.
}

// Our software components
BatteryPack batteryPack(MODULES_PER_PACK);
Shunt_ISA_iPace shunt;
Contactormanager contactor_manager;
BMS battery_manager(batteryPack, shunt, contactor_manager);

// Misc global variables
// int balancecount = 0;

void setup()
{
  Serial.begin(500000);
#ifdef DEBUG
  // Setup internal LED
  pinMode(LED_BUILTIN, OUTPUT);
  // Setup serial port
  SerialUSB1.begin(1000000);
  // SignalManager::setStream(SerialUSB1);
  while (!Serial)
  {
    delay(50);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  delay(100);
#endif

  Serial.println("Setup software modules:");

  // Setup SW components
  //-- only constructors go here. No setup if initialization methods

  // Brownout method here & Setup non-volatile memory

  // System functions startup
  scheduler.startNow();
  // scheduler.cpuLoadReset();
  enable_led_blink();
  enable_update_system_load();
  enable_BMS_monitor();

  // Sub-modules startup
  enable_update_shunt();
  enable_update_contactors();
  enable_handle_battery_CAN_messages();
  enable_poll_battery_for_data();

  // For the following code the init must be in the update. Init is always blocking
  // while ((shunt.getState() != Shunt_ISA_iPace::OPERATING) || (contactor_manager.getState() != Contactormanager::OPEN) || (batteryPack.getState() != BatteryPack::OPERATING))
  // {
  //   /* code */
  // }

  // Main module startup
  enable_BMS_tasks();
  enable_print_debug();
  enable_serial_console();

  // Watchdog startup
  // WDT_timings_t config;
  // // GEVCU might loop very rapidly sometimes so windowing mode would be tough. Revisit later
  // // config.window = 100; /* in milliseconds, 32ms to 522.232s, must be smaller than timeout */
  // config.timeout = WATCHDOG_TIMEOUT; /* in milliseconds, 32ms to 522.232s */
  // config.callback = wdtCallback;
  // wdt.begin(config);
}

void loop()
{
  scheduler.execute();
  serial_console();
  //Poll can buses

  // wdt.feed(); // must feed the watchdog every so often or it'll get angry
}

