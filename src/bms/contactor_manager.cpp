#include <Arduino.h>
#include "settings.h"
#include "bms/contactor_manager.h"
#include "bms/contactor.h"

Contactormanager::Contactormanager() : _negativeContactor(CONTACTOR_NEG_OUT_PIN, CONTACTOR_NEG_IN_PIN, CONTACTOR_DEBOUNCE, CONTACTOR_TIMEOUT),
    _prechargeContactor (CONTACTOR_PRCHG_OUT_PIN, CONTACTOR_PRCHG_IN_PIN, CONTACTOR_DEBOUNCE, CONTACTOR_TIMEOUT),
    _positiveContactor (CONTACTOR_POS_OUT_PIN, CONTACTOR_POS_IN_PIN, CONTACTOR_DEBOUNCE, CONTACTOR_TIMEOUT)
{
    _currentState = INIT;

}

void Contactormanager::initialise()
{
    _negativeContactor.initialise();
    _prechargeContactor.initialise();
    _positiveContactor.initialise();
    if (_negativeContactor.getState() == Contactor::FAULT ||
        _prechargeContactor.getState() == Contactor::FAULT ||
        _positiveContactor.getState() == Contactor::FAULT)
    {
        _currentState = FAULT;
    }
    else
    {
        _currentState = OPEN;
    }
}

void Contactormanager::close()
{
    _targetState = CLOSED;
}

void Contactormanager::open()
{
    _targetState = OPEN;
}

Contactormanager::State Contactormanager::getState()
{
    return _currentState;
}

void Contactormanager::update()
{
    // Update state of each contactor
    _positiveContactor.update(); // In case of an fault eg timeout we want to keep the opening order
    _prechargeContactor.update();
    _negativeContactor.update();

    // Determine state based on state of individual contactors
    if (_positiveContactor.getState() == Contactor::FAULT ||
        _prechargeContactor.getState() == Contactor::FAULT ||
        _negativeContactor.getState() == Contactor::FAULT)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _negativeContactor.open();
        _currentState = FAULT;
    }

    switch (_currentState)
    {
    case INIT:
        break;

    case OPEN:
        if (_targetState == CLOSED)
        {
            _negativeContactor.close();
            _currentState = CLOSING_NEGATIVE;
        }
        else if (_targetState == OPEN)
        {
        }
        break;

    case CLOSING_NEGATIVE:
        if (_targetState == CLOSED)
        {
            if (_negativeContactor.getState() == Contactor::CLOSED)
            {
                _prechargeContactor.close();
                _currentState = CLOSING_PRECHARGE;
            }
        }
        else if (_targetState == OPEN)
        {
            _negativeContactor.open();
            _currentState = OPENING_NEGATIVE;
        }
        break;

    case CLOSING_PRECHARGE:
        if (_targetState == CLOSED)
        {
            if (_prechargeContactor.getState() == Contactor::CLOSED)
            {
                _lastPreChangeTime = millis(); // Not directly close positive but wait
                _currentState = CLOSING_POSITIVE;
            }
        }
        else if (_targetState == OPEN)
        {
            _prechargeContactor.open();
            _currentState = OPENING_PRECHARGE;
        }
        break;

    case CLOSING_POSITIVE:
        if (_targetState == CLOSED)
        {
            if (millis() - _lastPreChangeTime > CONTACTOR_PRCHG_TIME) // This is the precharge delay criteria for closing
            {
                _positiveContactor.close();
            }

            if (_positiveContactor.getState() == Contactor::CLOSED)
            {
                _currentState = CLOSED;
            }
        }
        else if (_targetState == OPEN)
        {
            _positiveContactor.open();
            _currentState = OPENING_POSITIVE;
        }
        break;

    case CLOSED:
        if (_targetState == CLOSED)
        {
        }
        else if (_targetState == OPEN)
        {
            _positiveContactor.open();
            _currentState = OPENING_POSITIVE;
        }
        break;
    case OPENING_POSITIVE:
        if (_targetState == CLOSED)
        {
            // Opening procedure is never interrupted for closing again
        }
        else if (_targetState == OPEN)
        {
            if (_positiveContactor.getState() == Contactor::OPEN)
            {
                _lastPreChangeTime = millis(); // Not directly open precharge but wait
                _currentState = OPENING_PRECHARGE;
            }
        }
        break;
    case OPENING_PRECHARGE:
        if (_targetState == CLOSED)
        {
            // Opening procedure is never interrupted for closing again
        }
        else if (_targetState == OPEN)
        {
            if (millis() - _lastPreChangeTime > CONTACTOR_PRCHG_TIME) // This is the precharge delay criteria for opening
            {
                _prechargeContactor.open();
            }

            if (_prechargeContactor.getState() == Contactor::OPEN)
            {
                _currentState = OPENING_NEGATIVE;
            }
        }
        break;
    case OPENING_NEGATIVE:
        if (_targetState == CLOSED)
        {
            // Opening procedure is never interrupted for closing again
        }
        else if (_targetState == OPEN)
        {
            if (_negativeContactor.getState() == Contactor::OPEN)
            {
                _currentState = OPEN;
            }
        }
        break;
    case FAULT:
        _positiveContactor.open();
        _prechargeContactor.open();
        _negativeContactor.open();
        break;
    }
}
