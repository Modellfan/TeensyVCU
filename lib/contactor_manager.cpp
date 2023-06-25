#include <Arduino.h>
#include "settings.h"
#include "contactor_manager.h"

Contactormanager::Contactormanager(int negativeOutputPin, int negativeInputPin,
                                   int prechargeOutputPin, int prechargeInputPin,
                                   int positiveOutputPin, int positiveInputPin) {
    _negativeContactor(negativeOutputPin, negativeInputPin);
    _prechargeContactor(prechargeOutputPin, prechargeInputPin);
    _positiveContactor(positiveOutputPin, positiveInputPin);
    _currentState = INIT;
}

void Contactormanager::initialise() {
    _negativeContactor.initialise();
    _prechargeContactor.initialise();
    _positiveContactor.initialise();
    if (_negativeContactor.getState() == FAULT ||
        _prechargeContactor.getState() == FAULT ||
        _positiveContactor.getState() == FAULT) {
        _currentState = FAULT;
    } else {
        _currentState = OPEN;
    }
}

void Contactormanager::close() {
    switch (_currentState) {
        case OPEN:
        case OPENING_NEGATIVE:
            _currentState = CLOSING_NEGATIVE;
            _negativeContactor.close();
            break;
        case CLOSING_NEGATIVE:
            if (_negativeContactor.getState() == CLOSED) {
                _currentState = CLOSING_PRECHARGE;
                _prechargeContactor.close();
            }
            break;
        case CLOSING_PRECHARGE:
            if (_prechargeContactor.getState() == CLOSED) {
                _currentState = CLOSING_POSITIVE;
                _positiveContactor.close();
            }
            break;
        case CLOSING_POSITIVE:
            if (_positiveContactor.getState() == CLOSED) {
                _currentState = CLOSED;
            }
            break;
        case OPENING_PRECHARGE:
        case OPENING_POSITIVE:
        case CLOSED:
        case FAULT:
        default:
            break;
    }
}

void Contactormanager::open() {
    switch (_currentState) {
        case CLOSED:
        case OPENING_POSITIVE:
            _currentState = OPENING_POSITIVE;
            _positiveContactor.open();
            break;
        case OPENING_PRECHARGE:
            _prechargeContactor.open();
            if (_prechargeContactor.getState() == OPEN) {
                _currentState = OPENING_NEGATIVE;
                _negativeContactor.open();
            }
            break;
        case OPENING_NEGATIVE:
            if (_negativeContactor.getState() == OPEN) {
                _currentState = OPEN;
            }
            break;
        case OPEN:
        case CLOSING_NEGATIVE:
        case CLOSING_PRECHARGE:
        case CLOSING_POSITIVE:
        case FAULT:
        default:
            break;
    }
}

Contactormanager::State Contactormanager::getState() {
    return _currentState;
}


void Contactormanager::update()
{
    // Update state of each contactor
    _negContactor.update();
    _preContactor.update();
    _posContactor.update();

    // Determine state based on state of individual contactors
    if (_negContactor.getState() == Contactor::FAULT ||
        _preContactor.getState() == Contactor::FAULT ||
        _posContactor.getState() == Contactor::FAULT)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _negContactor.open();
        _preContactor.open();
        _posContactor.open();
        _currentState = FAULT;
        digitalWrite(_faultPin, HIGH);
    }
    else if (_negContactor.getState() == Contactor::OPEN &&
             _preContactor.getState() == Contactor::OPEN &&
             _posContactor.getState() == Contactor::OPEN)
    {
        // If all contactors are open, set manager state to OPEN
        _currentState = OPEN;
        digitalWrite(_faultPin, LOW);
    }
    else if (_negContactor.getState() == Contactor::CLOSING ||
             _preContactor.getState() == Contactor::CLOSING ||
             _posContactor.getState() == Contactor::CLOSING)
    {
        // If any contactor is closing, set manager state to CLOSING
        _currentState = CLOSING;
        digitalWrite(_faultPin, LOW);
    }
    else if (_negContactor.getState() == Contactor::CLOSED &&
             _preContactor.getState() == Contactor::OPEN &&
             _posContactor.getState() == Contactor::OPEN)
    {
        // If negative contactor is closed and precharge and positive contactors are open, 
        // start closing precharge contactor after a delay
        if (millis() - _lastNegCloseTime >= _negCloseDelay)
        {
            _preContactor.close();
            _lastPreCloseTime = millis();
        }
    }
    else if (_negContactor.getState() == Contactor::CLOSED &&
             _preContactor.getState() == Contactor::CLOSED &&
             _posContactor.getState() == Contactor::OPEN)
    {
        // If negative and precharge contactors are closed and positive contactor is open, 
        // start closing positive contactor after a delay
        if (millis() - _lastPreCloseTime >= _preCloseDelay)
        {
            _posContactor.close();
            _lastPosCloseTime = millis();
        }
    }
    else if (_negContactor.getState() == Contactor::CLOSED &&
             _preContactor.getState() == Contactor::CLOSED &&
             _posContactor.getState() == Contactor::CLOSED)
    {
        // If all contactors are closed, set manager state to CLOSED
        _currentState = CLOSED;
        digitalWrite(_faultPin, LOW);
    }
    else if (_negContactor.getState() == Contactor::OPENING ||
             _preContactor.getState() == Contactor::OPENING ||
             _posContactor.getState() == Contactor::OPENING)
    {
        // If any contactor is opening, set manager state to OPENING
        _currentState = OPENING;
        digitalWrite(_faultPin, LOW);
    }
    else
    {
        // Unknown state, set manager state to FAULT
        _currentState = FAULT;
        digitalWrite(_faultPin, HIGH);
    }
}

