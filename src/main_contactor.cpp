#include <Arduino.h>
#include <TaskScheduler.h>
#include <contactor.cpp>

#ifndef __IMXRT1062__
#error "This sketch should be compiled for Teensy 4.1"
#endif

// Define a Contactor instance
Contactor myContactor(2, 3, 10, 1000); // Example values for outputPin, inputPin, debounce_ms, timeout_ms

// Create a Scheduler instance
Scheduler scheduler;

// Define a function to call the update method of the Contactor instance
void updateContactor()
{
  myContactor.update();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(500000);

  // Initialize the Contactor instance
  myContactor.initialise();

  // Schedule the update function to run every 100ms
  scheduler.addTask(updateContactor);
  scheduler.setInterval(updateContactor, 100);
  scheduler.enableAll();
}

void loop()
{
  // Run the Scheduler instance
  scheduler.execute();
}