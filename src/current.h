#ifndef SHUNT_ISA_IPACE_H
#define SHUNT_ISA_IPACE_H

#include <ACAN_T4.h>
#include <Arduino.h>
#include "settings.h"

class Shunt_ISA_iPace
{

public:
    enum Shunt_ISA_iPace_State
    {
        INIT,      // Shunt is being initialized
        OPERATING, // Shunt is in operation state
        FAULT      // Shunt is in fault state
    };

    Shunt_ISA_iPace();

    void printFrame(CANMessage &frame);

    void initialise(); // For blocking singular initialize
    void update();     // Polls the can bus assigned in settings and saves the result. 10ms time slice.

    float getCurrent();
    float getTemperature();
    float getAmpereSeconds();
    float getCurrentAverage();
    float getCurrentDerivative();

private:
    Shunt_ISA_iPace_State _state;
    
    long _lastUpdate; // Time we received last update from BMS
    uint8_t _pollMessageId; // For counting if a message was skipped

    float _temperature;            // Internal temperature of shunt
    float _current;                // Current value
    float _current_average;        // Moving average over 1 second
    float _last_current;           // The current value, which was recieved before
    float _ampere_seconds;         // Integrated value
    unsigned long _previousMillis; // Previous time
    bool _firstUpdate;             // Flag for the first update
    float _current_derivative;     // Derivative of the current in Amps per second
 
    void calculate_current_values(); //Updates all values when a new current value is recieved
};

#endif
