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

    typedef enum
    {
        DTC_CON_NONE = 0,
        DTC_CON_INIT_CLOSED = 1 << 0,
        DTC_CON_UNEXPECTED_CLOSED = 1 << 1,
        DTC_CON_UNEXPECTED_OPEN = 1 << 2,
        DTC_CON_OPEN_TIMEOUT = 1 << 3,
        DTC_CON_CLOSE_TIMEOUT = 1 << 4,
    } DTC_CON;

    Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms,
              bool allowExternalControl = false);

    void initialise();
    void close();
    void open();
    State getState() const;
    bool getInputPin() const;
    bool getOutputPin() const;
    DTC_CON getDTC() const;
    String getDTCString() const;
    void update();

private:
    byte _outputPin;
    byte _inputPin;
    unsigned int _debounce_ms;
    unsigned int _timeout_ms;
    unsigned long _lastStateChange;
    State _currentState;
    DTC_CON _dtc;
    bool _allowExternalControl;
};

#endif
