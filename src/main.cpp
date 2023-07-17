#include <Arduino.h>
#include <ACAN_T4.h>

#include <TaskScheduler.h>

#include <current.h>
#include <contactor.h>
#include "contactor_manager.h"
#include "battery i3/pack.h"

#include "comms.h"

#ifndef __IMXRT1062__
#error "This sketch should be compiled for Teensy 4.1"
#endif

// Create a Scheduler instance
Scheduler scheduler;
// State state;
// StatusLight statusLight;

// Battery battery(NUM_PACKS);

BatteryPack batteryPack(8, 12, 4);
// MCP2515 mainCAN(SPI_PORT, MAIN_CAN_CS, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
Shunt_ISA_iPace shunt;

// // Define a Contactor instance
// Contactor myContactor(2, 3, 100, 200); // Example values for outputPin, inputPin, debounce_ms, timeout_ms
Contactormanager contactor_manager;

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
  while (!Serial)
  {
    delay(50);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  delay(2000);

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

  enable_led_blink();
  enable_update_system_load();
  // enable_update_shunt();
  // enable_update_contactors();
  //enable_handle_battery_CAN_messages();
  //enable_poll_battery_for_data();
  
  //-> Update shunt until in State = Operating . Then start initialzing contactors

  //   printf("Enabling handling of inbound CAN messages from batteries\n");
  // enable_handle_battery_CAN_messages();

  // printf("Enabling module polling\n");
  // enable_module_polling();

  // printf("Enabling status print\n");
  // enable_status_print();

  // printf("Enable listen for IGNITION_ON signal\n");
  // enable_listen_for_ignition_signal();

  // printf("Enable listen for CHARGE_ENABLE signal\n");
  // enable_listen_for_charge_enable_signal();
}

void loop()
{
  scheduler.execute();
}