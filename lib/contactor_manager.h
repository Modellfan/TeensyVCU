#ifndef CONTACTORMANAGER_H
#define CONTACTORMANAGER_H

#include <Arduino.h>
#include "settings.h"
#include "contactor.h"

class Contactormanager {
public:
    typedef enum State {
        INIT,                 // Contactors are being initialized
        OPEN,                 // Contactors are open
        OPENING_POSITIVE,     // Positive contactor is opening
        OPENING_PRECHARGE,    // Precharge contactor is opening
        OPENING_NEGATIVE,     // Negative contactor is opening
        CLOSING_NEGATIVE,     // Negative contactor is closing
        CLOSING_PRECHARGE,    // Precharge contactor is closing
        CLOSING_POSITIVE,     // Positive contactor is closing
        CLOSED,               // Contactors are closed
        FAULT                 // Contactors are in fault state
    };

    Contactormanager(int negativeOutputPin, int negativeInputPin,
                     int prechargeOutputPin, int prechargeInputPin,
                     int positiveOutputPin, int positiveInputPin);

    void initialise();
    void close();
    void open();
    State getState();
    void update();

private:
    Contactor _negativeContactor;
    Contactor _positiveContactor;
    Contactor _prechargeContactor;
    unsigned long _lastStateChange;
    State _currentState;
    State _closingState;
    State _openingState;
};

#endif  // CONTACTORMANAGER_H
