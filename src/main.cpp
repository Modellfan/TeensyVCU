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
BatteryPack batteryPack(8);
Shunt_ISA_iPace shunt;
Contactormanager contactor_manager;
BMS battery_manager(batteryPack, shunt, contactor_manager);

// Misc global variables
int balancecount = 0;

const int16_t xs[] PROGMEM = {300, 700, 800, 900, 1500, 1800, 2100, 2500};
const int8_t ys[] PROGMEM = {-127, -50, 127, 0, 10, -30, -50, 10};
const byte ysb[] PROGMEM = {0, 30, 55, 89, 99, 145, 255, 10};
const float ysfl[] PROGMEM = {-127.3, -49.9, 127.0, 0.0, 13.3, -33.0, -35.8, 10.0};

void setup()
{
#ifdef DEBUG
  // Setup internal LED
  pinMode(LED_BUILTIN, OUTPUT);

  // Setup serial port
  Serial.begin(500000);
  SerialUSB1.begin(1000000);
  //SignalManager::setStream(SerialUSB1);
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
  // wdt.feed(); // must feed the watchdog every so often or it'll get angry
}

// Map2D<8, int16_t, int8_t> test;
// test.setXs_P(xs);
// test.setYs_P(ys);

// for (int idx = 250; idx < 2550; idx += 50)
// {
//   int8_t val = test.f(idx);
//   Serial.print(idx);
//   Serial.print(F(": "));
//   Serial.println((int)val);
// }

