#include <Arduino.h>
#include "contactor.h"
#include "settings.h"

Contactor::Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms,
                     bool allowExternalControl)
    : _outputPin(outputPin),
      _inputPin(inputPin),
      _debounce_ms(debounce_ms),
      _timeout_ms(timeout_ms),
      _currentState(INIT),
      _allowExternalControl(allowExternalControl),
      _feedbackDisabled(false)
{
    pinMode(_outputPin, OUTPUT);
    pinMode(_inputPin, INPUT_PULLUP);
    digitalWrite(_outputPin, LOW);
    _lastStateChange = millis();
    _dtc = DTC_CON_NONE;
}

void Contactor::initialise()
{
    if (_feedbackDisabled)
    {
        if (_currentState == INIT)
        {
            _dtc = DTC_CON_NONE;
            digitalWrite(_outputPin, LOW);
            _currentState = OPEN;
            _lastStateChange = millis();
        }
        return;
    }

    if (_currentState == INIT)
    {
        _dtc = DTC_CON_NONE;
        digitalWrite(_outputPin, LOW);
        if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
        {
            if (_allowExternalControl)
            {
                _currentState = CLOSED;
                _lastStateChange = millis();
            }
            else
            {
                _currentState = FAULT;
                digitalWrite(_outputPin, LOW);
                _dtc = DTC_CON_INIT_CLOSED;
            }
        }
        else
        {
            _currentState = OPEN;
            _lastStateChange = millis();
        }
    }
}

void Contactor::close()
{
    if (_feedbackDisabled)
    {
        digitalWrite(_outputPin, HIGH);
        _currentState = CLOSED;
        _lastStateChange = millis();
        _dtc = DTC_CON_NONE;
        return;
    }

    if (_currentState == OPEN || _currentState == OPENING)
    {
        _currentState = CLOSING;
        _lastStateChange = millis();
        digitalWrite(_outputPin, HIGH);
    }
}

void Contactor::open()
{
    if (_feedbackDisabled)
    {
        digitalWrite(_outputPin, LOW);
        _currentState = OPEN;
        _lastStateChange = millis();
        _dtc = DTC_CON_NONE;
        return;
    }

    if (_currentState == CLOSED || _currentState == CLOSING)
    {
        _currentState = OPENING;
        _lastStateChange = millis();
        digitalWrite(_outputPin, LOW);
    }
}

Contactor::State Contactor::getState() const
{
    return _currentState;
}

bool Contactor::getInputPin() const
{
    return (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE);
}

bool Contactor::getOutputPin() const
{
    return digitalRead(_outputPin);
}

void Contactor::update()
{
    if (_feedbackDisabled)
    {
        return;
    }

    switch (_currentState)
    {
    case INIT:
        break;
    case OPEN:
        if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
        {
            if (_allowExternalControl)
            {
                _currentState = CLOSED;
                _lastStateChange = millis();
            }
            else
            {
                _currentState = FAULT;
                digitalWrite(_outputPin, LOW);
                _dtc = DTC_CON_UNEXPECTED_CLOSED;
            }
        }
        break;
    case CLOSED:
        if (digitalRead(_inputPin) != CONTACTOR_CLOSED_STATE)
        {
            if (_allowExternalControl)
            {
                _currentState = OPEN;
                _lastStateChange = millis();
            }
            else
            {
                _currentState = FAULT;
                digitalWrite(_outputPin, LOW);
                _dtc = DTC_CON_UNEXPECTED_OPEN;
            }
        }
        break;
    case OPENING:
        if ((millis() - _lastStateChange) > _timeout_ms)
        {
            _currentState = FAULT;
            digitalWrite(_outputPin, LOW);
            _dtc = DTC_CON_OPEN_TIMEOUT;
        }
        else
        {
            if ((millis() - _lastStateChange) > _debounce_ms)
            {
                if (digitalRead(_inputPin) != CONTACTOR_CLOSED_STATE)
                {
                    _currentState = OPEN;
                    _lastStateChange = millis();
                }
            }
        }
        break;
    case CLOSING:
        if ((millis() - _lastStateChange) > _timeout_ms)
        {
            _currentState = FAULT;
            digitalWrite(_outputPin, LOW);
            _dtc = DTC_CON_CLOSE_TIMEOUT;
        }
        else
        {
            if ((millis() - _lastStateChange) > _debounce_ms)
            {
                if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
                {
                    _currentState = CLOSED;
                    _lastStateChange = millis();
                }
            }
        }
        break;
    case FAULT:
        digitalWrite(_outputPin, LOW);
        break;
    }

}

Contactor::DTC_CON Contactor::getDTC() const { return _dtc; }

void Contactor::setFeedbackDisabled(bool disabled)
{
    if (_feedbackDisabled == disabled)
    {
        return;
    }

    _feedbackDisabled = disabled;

    if (_feedbackDisabled)
    {
        _dtc = DTC_CON_NONE;
        _lastStateChange = millis();
        _currentState = digitalRead(_outputPin) == CONTACTOR_CLOSED_STATE ? CLOSED : OPEN;
    }
    else
    {
        _dtc = DTC_CON_NONE;
        _currentState = INIT;
        initialise();
    }
}

String Contactor::getDTCString() const
{
    String errorString = "";

    if (_dtc == DTC_CON_NONE)
    {
        errorString = "None";
    }
    else
    {
        bool hasError = false;

        if (_dtc & DTC_CON_INIT_CLOSED)
        {
            errorString += "INIT_CLOSED, ";
            hasError = true;
        }
        if (_dtc & DTC_CON_UNEXPECTED_CLOSED)
        {
            errorString += "UNEXPECTED_CLOSED, ";
            hasError = true;
        }
        if (_dtc & DTC_CON_UNEXPECTED_OPEN)
        {
            errorString += "UNEXPECTED_OPEN, ";
            hasError = true;
        }
        if (_dtc & DTC_CON_OPEN_TIMEOUT)
        {
            errorString += "OPEN_TIMEOUT, ";
            hasError = true;
        }
        if (_dtc & DTC_CON_CLOSE_TIMEOUT)
        {
            errorString += "CLOSE_TIMEOUT, ";
            hasError = true;
        }

        if (hasError)
        {
            errorString.remove(errorString.length() - 2);
        }
    }

    return errorString;
}
