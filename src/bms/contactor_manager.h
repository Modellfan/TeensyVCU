#ifndef CONTACTORMANAGER_H
#define CONTACTORMANAGER_H

#include <Arduino.h>
#include "settings.h"
#include "bms/contactor.h"

class Contactormanager
{
public:
    enum State
    {
        INIT, // Contactors are being initialized
        OPEN, // Contactors are open

        CLOSING_NEGATIVE,  // Negative contactor is closing
        CLOSING_PRECHARGE, // Precharge contactor is closing
        CLOSING_POSITIVE,  // Positive contactor is closing

        CLOSED, // Contactors are closed

        OPENING_POSITIVE,  // Positive contactor is opening
        OPENING_PRECHARGE, // Precharge contactor is opening
        OPENING_NEGATIVE,  // Negative contactor is opening

        FAULT // Contactors are in fault state
    };

    Contactormanager();

    void initialise();
    void close();
    void open();
    State getState();
    void update();

private:
    Contactor _negativeContactor;
    Contactor _positiveContactor;
    Contactor _prechargeContactor;
    unsigned long _lastPreChangeTime;
    State _currentState;
    State _targetState;
};

#endif // CONTACTORMANAGER_H
