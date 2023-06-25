#ifndef CONTACTOR_H
#define CONTACTOR_H

#include <Arduino.h>
#include "settings.h"

class Contactor
{
public:
    enum State
    {
        INIT,
        OPEN,
        CLOSING,
        CLOSED,
        OPENING,
        FAULT
    };

    Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms);

    void initialise();
    void close();
    void open();
    State getState();
    void update();

private:
    byte _outputPin;
    byte _inputPin;
    unsigned int _debounce_ms;
    unsigned int _timeout_ms;
    unsigned long _lastStateChange;
    State _currentState;
};

#endif
