#ifndef SHUNT_ISA_IPACE_H
#define SHUNT_ISA_IPACE_H

#include <ACAN_T4.h>
#include <Arduino.h>
#include "settings.h"
#include <functional> // Add this line to include the <functional> header

class Shunt_ISA_iPace
{

public:
    enum STATE_ISA
    {
        INIT,      // CMU is being initialized
        OPERATING, // CMU is in operation state
        FAULT      // CMU is in fault state
    };

    typedef enum
    {
        DTC_ISA_NONE = 0,
        DTC_ISA_CAN_INIT_ERROR = 1 << 0,
        DTC_ISA_TEMPERATURE_TOO_HIGH = 1 << 1,
        DTC_ISA_MAX_CURRENT_EXCEEDED = 1 << 2,
        DTC_ISA_TIMED_OUT = 1 << 3,
    } DTC_ISA;

    // Constructor
    Shunt_ISA_iPace();

    // Runnables
    void initialise(); // For blocking singular initialize
    void update();     // Polls the can bus assigned in settings and saves the result. 10ms time slice.

    // Our state
    STATE_ISA getState();
    DTC_ISA getDTC();

    // Our Signals
    float getCurrent();
    float getTemperature();
    float getAmpereSeconds();
    float getCurrentAverage();
    float getCurrentDerivative();

private:
    const char *getStateString();
    String getDTCString();

    STATE_ISA _state;
    DTC_ISA _dtc;

    long _lastUpdate; // Time we received last update from BMS
    uint8_t _0x3c3_framecounter;
    uint8_t _0x3D2_framecounter;
    uint8_t _last_status_bits;

    float _temperature;            // Internal temperature of shunt
    float _current;                // Current value
    float _current_average;        // Moving average over 1 second
    float _last_current;           // The current value, which was recieved before
    float _ampere_seconds;         // Integrated value
    unsigned long _previousMillis; // Previous time
    bool _firstUpdate;             // Flag for the first update
    float _current_derivative;     // Derivative of the current in Amps per second

    void calculate_current_values(); // Updates all values when a new current value is recieved
};

#endif
