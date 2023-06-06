#include <Arduino.h>
#include <TaskScheduler.h>
#include <ACAN_T4.h>

#include "comms.h"

#ifndef __IMXRT1062__
#error "This sketch should be compiled for Teensy 4.1"
#endif

// Create a Scheduler instance
#define _TASK_TIMECRITICAL
Scheduler scheduler;
// State state;
// StatusLight statusLight;

// Battery battery(NUM_PACKS);
// MCP2515 mainCAN(SPI_PORT, MAIN_CAN_CS, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);

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

  // Setup scheduler
  // scheduler.startNow();
  // scheduler.delay();
  // scheduler.cpuLoadReset()

  // Setup non-volatile memory
  //---

  // Setup SW components
  //-- only constructors go here. No setup if initialization methods

  enable_led_blink();
  enable_poll_can();
  
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

// void printFrame(CANMessage &frame)
// {
//   // Print message
//   Serial.print("ID: ");
//   Serial.print(frame.id, HEX);
//   Serial.print(" Ext: ");
//   if (frame.ext)
//   {
//     Serial.print("Y");
//   }
//   else
//   {
//     Serial.print("N");
//   }
//   Serial.print(" Len: ");
//   Serial.print(frame.len, DEC);
//   Serial.print(" ");
//   for (int i = 0; i < frame.len; i++)
//   {
//     Serial.print(frame.data[i], HEX);
//     Serial.print(" ");
//   }
//   Serial.println();
// }
