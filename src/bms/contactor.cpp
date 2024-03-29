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
            //Serial.println("State: Fault");
            digitalWrite(_outputPin, LOW);
            //Serial.println("Fault: input signal high at init");
        }
        else
        {
            _currentState = OPEN;
            //Serial.println("State: Open");
            _lastStateChange = millis();
        }
    }
    else
    {
        //Serial.println("Fault: Contactor was already initialized. A second initialize is not possible.");
    }
}

void Contactor::close()
{
    if (_currentState == OPEN || _currentState == OPENING)
    {
        _currentState = CLOSING;
        //Serial.println("State: Closing");
        _lastStateChange = millis();
        digitalWrite(_outputPin, HIGH);
    }
}

void Contactor::open()
{
    if (_currentState == CLOSED || _currentState == CLOSING)
    {
        _currentState = OPENING;
        //Serial.println("State: Opening");
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
    // PLOT("Contactor Input",digitalRead(_inputPin));
    // PLOT("Contactor Output",digitalRead(_outputPin));

    switch (_currentState)
    {
    case INIT:
        break;
    case OPEN:
        if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
        {
            _currentState = FAULT;
            //Serial.println("State: Fault");
            digitalWrite(_outputPin, LOW);
        }
        break;
    case CLOSED:
        if (digitalRead(_inputPin) != CONTACTOR_CLOSED_STATE)
        {
            _currentState = FAULT;
            ///Serial.println("State: Fault");
            digitalWrite(_outputPin, LOW);
        }
        break;
    case OPENING:
        if ((millis() - _lastStateChange) > _timeout_ms)
        {
            _currentState = FAULT;
            //Serial.println("State: Fault");
            digitalWrite(_outputPin, LOW);
        }
        else
        {
            if ((millis() - _lastStateChange) > _debounce_ms)
            {
                if (digitalRead(_inputPin) != CONTACTOR_CLOSED_STATE)
                {
                    _currentState = OPEN;
                    //Serial.println("State: Open");
                    _lastStateChange = millis();
                }
            }
        }
        break;
    case CLOSING:
        if ((millis() - _lastStateChange) > _timeout_ms)
        {
            _currentState = FAULT;
            //Serial.println(millis());
            //Serial.println(_lastStateChange);
            //Serial.println("State: Fault");
            digitalWrite(_outputPin, LOW);
        }
        else
        {
            if ((millis() - _lastStateChange) > _debounce_ms)
            {
                if (digitalRead(_inputPin) == CONTACTOR_CLOSED_STATE)
                {
                    _currentState = CLOSED;
                    //Serial.println("State: Closed");
                    _lastStateChange = millis();
                }
            }
        }
        break;
    case FAULT:
        digitalWrite(_outputPin, LOW);
        break;
    }

    // PLOT("Contactor State",_currentState);
}
