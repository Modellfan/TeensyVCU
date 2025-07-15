#include <Arduino.h>
#include "contactor.h"
#include "settings.h"

Contactor::Contactor(int outputPin, int inputPin, int debounce_ms, int timeout_ms)
    : _outputPin(outputPin),
      _inputPin(inputPin),
      _debounce_ms(debounce_ms),
      _timeout_ms(timeout_ms),
      _currentState(INIT)
{
    pinMode(_outputPin, OUTPUT);
    pinMode(_inputPin, INPUT_PULLUP);
    digitalWrite(_outputPin, LOW);
    _lastStateChange = millis();
}

void Contactor::initialise()
{
    if (_currentState == INIT)
    {
        digitalWrite(_outputPin, LOW);
        if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
        {
            _currentState = FAULT;
            digitalWrite(_outputPin, LOW);
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
    if (_currentState == OPEN || _currentState == OPENING)
    {
        _currentState = CLOSING;
        _lastStateChange = millis();
        digitalWrite(_outputPin, HIGH);
    }
}

void Contactor::open()
{
    if (_currentState == CLOSED || _currentState == CLOSING)
    {
        _currentState = OPENING;
        _lastStateChange = millis();
        digitalWrite(_outputPin, LOW);
    }
}

Contactor::State Contactor::getState()
{
    return _currentState;
}

bool Contactor::getInputPin()
{

    return (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE);
}

void Contactor::update()
{

    switch (_currentState)
    {
    case INIT:
        break;
    case OPEN:
        if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
        {
            _currentState = FAULT;
            digitalWrite(_outputPin, LOW);
        }
        break;
    case CLOSED:
        if (digitalRead(_inputPin) != CONTACTOR_CLOSED_STATE)
        {
            _currentState = FAULT;
            digitalWrite(_outputPin, LOW);
        }
        break;
    case OPENING:
        if ((millis() - _lastStateChange) > _timeout_ms)
        {
            _currentState = FAULT;
            digitalWrite(_outputPin, LOW);
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
