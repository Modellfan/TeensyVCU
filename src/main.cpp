#include <Arduino.h>
#include <ACAN_T4.h>

#include <TaskScheduler.h>
#include "utils/signalManager.h"
#include "utils/Map2D3D.h"
#include <Watchdog_t4.h>

#include <bms/current.h>
#include <bms/contactor.h>
#include "bms/contactor_manager.h"
#include "bms/battery i3/pack.h"
#include "bms/battery_manager.h"

#include "comms.h"

#ifndef __IMXRT1062__
#error "This sketch should be compiled for Teensy 4.1"
#endif

#define DEBUG

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
BMS battery_manager(batteryPack, shunt);

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
  SignalManager::setStream(SerialUSB1);
  while (!Serial)
  {
    delay(50);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  delay(100);
#endif

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

  Serial.println("Setup software modules:");

  // Setup scheduler
  scheduler.startNow();

  // scheduler.cpuLoadReset();

  // pinMode(28, INPUT_PULLUP);
  //  if (digitalRead(28)) {
  //     Serial.println("High");
  //   } else {
  //     Serial.println("Low");
  //   }
  // Setup non-volatile memory
  //---

  // Setup SW components
  //-- only constructors go here. No setup if initialization methods

  // Brownout method here

  // Startup procedure here
  enable_led_blink();
  enable_update_system_load();
  enable_update_shunt();
  //-> Update shunt until in State = Operating . Then start initialzing contactors
  // enable_update_contactors();
  enable_handle_battery_CAN_messages();
  enable_poll_battery_for_data();

  // Startup BMS module
  //enable_handle_bms_CAN_messages();
  enable_BMS_tasks();
  enable_BMS_monitor();

  enable_print_debug();

  // Setup watchdog
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