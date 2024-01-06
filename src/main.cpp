#include <Arduino.h>
#include <ACAN_T4.h>

#include <TaskScheduler.h>
#include "utils/signalManager.h"
#include "utils/Map2D3D.h"

#include <bms/current.h>
#include <bms/contactor.h>
#include "bms/contactor_manager.h"
#include "bms/battery i3/pack.h"

#include "comms.h"

#ifndef __IMXRT1062__
#error "This sketch should be compiled for Teensy 4.1"
#endif

// Create a Scheduler instance
Scheduler scheduler;
// State state;
// StatusLight statusLight;

// Our software components
BatteryPack batteryPack(8, 12, 4);
Shunt_ISA_iPace shunt;
Contactormanager contactor_manager;

// Misc global variables
int balancecount = 0;

const int16_t xs[] PROGMEM = {300, 700, 800, 900, 1500, 1800, 2100, 2500};
const int8_t ys[] PROGMEM = {-127, -50, 127, 0, 10, -30, -50, 10};
const byte ysb[] PROGMEM = {0, 30, 55, 89, 99, 145, 255, 10};
const float ysfl[] PROGMEM = {-127.3, -49.9, 127.0, 0.0, 13.3, -33.0, -35.8, 10.0};

// bool watchdog_keepalive(struct repeating_timer *t) {
//     watchdog_update();
//     return true;
// }

// void enable_watchdog_keepalive() {
//     add_repeating_timer_ms(5000, watchdog_keepalive, NULL, &watchdogKeepaliveTimer);
// }

void setup()
{
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

  Map2D<8, int16_t, int8_t> test;
  test.setXs_P(xs);
  test.setYs_P(ys);

  for (int idx = 250; idx < 2550; idx += 50)
  {
    int8_t val = test.f(idx);
    Serial.print(idx);
    Serial.print(F(": "));
    Serial.println((int)val);
  }

  Serial.println("Fct: Setup");

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
  // enable_update_shunt();
  // enable_update_contactors();
  enable_handle_battery_CAN_messages();
  enable_poll_battery_for_data();

  enable_print_debug();
}

void loop()
{
  scheduler.execute();
  //-> Update shunt until in State = Operating . Then start initialzing contactors
}