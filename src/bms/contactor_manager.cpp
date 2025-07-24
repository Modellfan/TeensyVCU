#include <Arduino.h>
#include "settings.h"
#include <ACAN_T4.h>
#include "bms/contactor_manager.h"
#include "bms/contactor.h"
#include "utils/can_packer.h"

Contactormanager::Contactormanager() :
    _prechargeContactor(CONTACTOR_PRCHG_OUT_PIN,
                        CONTACTOR_PRCHG_IN_PIN,
                        CONTACTOR_DEBOUNCE,
                        CONTACTOR_TIMEOUT,
                        true),
    _positiveContactor(CONTACTOR_POS_OUT_PIN,
                       CONTACTOR_POS_IN_PIN,
                       CONTACTOR_DEBOUNCE,
                       CONTACTOR_TIMEOUT)
{
    _currentState = INIT;
}

void Contactormanager::initialise()
{
    pinMode(CONTACTOR_POWER_SUPPLY_IN_PIN, INPUT_PULLUP);
    pinMode(CONTACTOR_NEG_IN_PIN, INPUT_PULLUP);

    _negativeContactor_closed = (digitalRead(CONTACTOR_NEG_IN_PIN) == CONTACTOR_CLOSED_STATE);
    _contactorVoltage_available = (digitalRead(CONTACTOR_POWER_SUPPLY_IN_PIN) == CONTACTOR_CLOSED_STATE);

    _prechargeContactor.initialise();
    _positiveContactor.initialise();

    if (_positiveContactor.getState() == Contactor::FAULT)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_POSITIVE_CONTACTOR_FAULT;
        _currentState = FAULT;
    }

    if (_prechargeContactor.getState() == Contactor::FAULT)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_PRECHARGE_CONTACTOR_FAULT;
        _currentState = FAULT;
    }

    if (_negativeContactor_closed == false)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_NEGATIVE_CONTACTOR_FAULT;
        _currentState = FAULT;
    }

    if (_contactorVoltage_available == false)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_NO_CONTACTOR_POWER_SUPPLY;
        _currentState = FAULT;
    }

    if (_currentState == INIT) // If no fault state set until here

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
    _negativeContactor_closed = (digitalRead(CONTACTOR_NEG_IN_PIN) == CONTACTOR_CLOSED_STATE);
    _contactorVoltage_available = (digitalRead(CONTACTOR_POWER_SUPPLY_IN_PIN) == CONTACTOR_CLOSED_STATE);

    // Determine state based on state of individual contactors
    if (_positiveContactor.getState() == Contactor::FAULT)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_POSITIVE_CONTACTOR_FAULT;
        _currentState = FAULT;
    }

    if (_prechargeContactor.getState() == Contactor::FAULT)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_PRECHARGE_CONTACTOR_FAULT;
        _currentState = FAULT;
    }

    if (_negativeContactor_closed == false)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_NEGATIVE_CONTACTOR_FAULT;
        _currentState = FAULT;
    }

    if (_contactorVoltage_available == false)
    {
        // If any contactor is in fault state, put all contactors in open state and set manager state to fault
        _positiveContactor.open();
        _prechargeContactor.open();
        _dtc = DTC_COM_NO_CONTACTOR_POWER_SUPPLY;
        _currentState = FAULT;
    }

    switch (_currentState)
    {
    case INIT:
        break;

    case OPEN:
        if (_targetState == CLOSED)
        {
            _prechargeContactor.close();
            _currentState = CLOSING_PRECHARGE;
        }
        else if (_targetState == OPEN)
        {
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
                _currentState = OPEN;
            }
        }
        break;
    case FAULT:
        _positiveContactor.open();
        _prechargeContactor.open();
        break;
    }
}

const char *Contactormanager::getCurrentStateString()
{
    switch (_currentState)
    {
    case INIT:
        return "INIT";
    case OPEN:
        return "OPEN";
    case CLOSING_PRECHARGE:
        return "CLOSING_PRECHARGE";
    case CLOSING_POSITIVE:
        return "CLOSING_POSITIVE";
    case CLOSED:
        return "CLOSED";
    case OPENING_POSITIVE:
        return "OPENING_POSITIVE";
    case OPENING_PRECHARGE:
        return "OPENING_PRECHARGE";
    case FAULT:
        return "FAULT";
    default:
        return "UNKNOWN STATE";
    }
}

const char *Contactormanager::getTargetStateString()
{
    switch (_targetState)
    {
    case INIT:
        return "INIT";
    case OPEN:
        return "OPEN";
    case CLOSING_PRECHARGE:
        return "CLOSING_PRECHARGE";
    case CLOSING_POSITIVE:
        return "CLOSING_POSITIVE";
    case CLOSED:
        return "CLOSED";
    case OPENING_POSITIVE:
        return "OPENING_POSITIVE";
    case OPENING_PRECHARGE:
        return "OPENING_PRECHARGE";
    case FAULT:
        return "FAULT";
    default:
        return "UNKNOWN STATE";
    }
}

String Contactormanager::getDTCString()
{
    String errorString = "";

    if (_dtc == DTC_COM_NONE)
    {
        errorString = "None";
    }
    else
    {
        bool hasError = false;

        if (_dtc & DTC_COM_NO_CONTACTOR_POWER_SUPPLY)
        {
            errorString += "DTC_COM_NO_CONTACTOR_POWER_SUPPLY, ";
            hasError = true;
        }
        if (_dtc & DTC_COM_NEGATIVE_CONTACTOR_FAULT)
        {
            errorString += "DTC_COM_NEGATIVE_CONTACTOR_FAULT, ";
            hasError = true;
        }
        if (_dtc & DTC_COM_PRECHARGE_CONTACTOR_FAULT)
        {
            errorString += "DTC_COM_PRECHARGE_CONTACTOR_FAULT, ";
            hasError = true;
        }
        if (_dtc & DTC_COM_POSITIVE_CONTACTOR_FAULT)
        {
            errorString += "DTC_COM_POSITIVE_CONTACTOR_FAULT, ";
            hasError = true;
        }

        if (hasError)
        {
            // Remove the trailing comma and space
            errorString.remove(errorString.length() - 2);
        }
    }
    return errorString;
}

Contactormanager::DTC_COM Contactormanager::getDTC() { return _dtc; }

Contactor::State Contactormanager::getPositiveState() const
{
    return _positiveContactor.getState();
}

bool Contactormanager::getPositiveInputPin() const
{
    return _positiveContactor.getInputPin();
}

Contactor::State Contactormanager::getPrechargeState() const
{
    return _prechargeContactor.getState();
}

bool Contactormanager::getPrechargeInputPin() const
{
    return _prechargeContactor.getInputPin();
}

bool Contactormanager::isNegativeContactorClosed() const
{
    return _negativeContactor_closed;
}

bool Contactormanager::isContactorVoltageAvailable() const
{
    return _contactorVoltage_available;
}
