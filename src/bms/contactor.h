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

    Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms,
              bool allowExternalControl = false);

    void initialise();
    void close();
    void open();
    State getState() const;
    bool getInputPin() const;
    bool getOutputPin() const;
    void update();

private:
    byte _outputPin;
    byte _inputPin;
    unsigned int _debounce_ms;
    unsigned int _timeout_ms;
    unsigned long _lastStateChange;
    State _currentState;
    bool _allowExternalControl;
};

#endif
