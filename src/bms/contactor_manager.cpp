#include <Arduino.h>
#include "settings.h"
#include <ACAN_T4.h>
#include "bms/contactor_manager.h"
#include "bms/contactor.h"
#include "utils/can_packer.h"
#include <cmath>

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
    _targetState = OPEN;
    _hvBusVoltage_v = 0.0f;
    _hvBusVoltage_valid = false;
    _hvBusVoltage_lastUpdateMs = 0UL;
    _packVoltage_v = 0.0f;
    _packVoltage_valid = false;
    _packVoltage_lastUpdateMs = 0UL;
    _prechargeStrategy = PrechargeStrategy::TIMED_DELAY;
    _voltageMatchTolerance_v = CONTACTOR_PRECHARGE_VOLTAGE_TOLERANCE;
    _voltageMatchTimeout_ms = CONTACTOR_PRECHARGE_MATCH_TIMEOUT_MS;
    _feedbackDisabled = false;
    _dtc = DTC_COM_NONE;
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
        enterFaultState(DTC_COM_POSITIVE_CONTACTOR_FAULT);
    }

    if (_prechargeContactor.getState() == Contactor::FAULT)
    {
        enterFaultState(DTC_COM_PRECHARGE_CONTACTOR_FAULT);
    }

    if (_negativeContactor_closed == false)
    {
        enterFaultState(DTC_COM_NEGATIVE_CONTACTOR_FAULT);
    }

    if (_contactorVoltage_available == false)
    {
        enterFaultState(DTC_COM_NO_CONTACTOR_POWER_SUPPLY);
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
        enterFaultState(DTC_COM_POSITIVE_CONTACTOR_FAULT);
    }

    if (_prechargeContactor.getState() == Contactor::FAULT)
    {
        enterFaultState(DTC_COM_PRECHARGE_CONTACTOR_FAULT);
    }

    if (_negativeContactor_closed == false)
    {
        enterFaultState(DTC_COM_NEGATIVE_CONTACTOR_FAULT);
    }

    if (_contactorVoltage_available == false)
    {
        enterFaultState(DTC_COM_NO_CONTACTOR_POWER_SUPPLY);
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
            bool faultTriggered = false;

            switch (_prechargeStrategy)
            {
            case PrechargeStrategy::TIMED_DELAY:
                faultTriggered = handleClosingPositiveTimedDelay();
                break;
            case PrechargeStrategy::VOLTAGE_MATCH:
                faultTriggered = handleClosingPositiveVoltageMatch();
                break;
            default:
                faultTriggered = handleClosingPositiveTimedDelay();
                break;
            }

            if (faultTriggered || _currentState == FAULT)
            {
                break;
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

void Contactormanager::enterFaultState(DTC_COM dtc)
{
    _positiveContactor.open();
    _prechargeContactor.open();
    _dtc = dtc;
    _currentState = FAULT;
    _targetState = OPEN;
}

bool Contactormanager::handleClosingPositiveTimedDelay()
{
    if (millis() - _lastPreChangeTime > CONTACTOR_PRCHG_TIME)
    {
        _positiveContactor.close();
    }

    return false;
}

bool Contactormanager::handleClosingPositiveVoltageMatch()
{
    const unsigned long now = millis();
    const unsigned long elapsed = now - _lastPreChangeTime;
    const unsigned long timeout_ms = static_cast<unsigned long>(_voltageMatchTimeout_ms);
    const unsigned long hv_timeout_ms = static_cast<unsigned long>(BMS_VCU_TIMEOUT);
    const unsigned long pack_timeout_ms =
        static_cast<unsigned long>(CONTACTOR_PACK_VOLTAGE_VALIDITY_MS);

    if (!isHvBusVoltageValid())
    {
        if (elapsed > hv_timeout_ms)
        {
            enterFaultState(DTC_COM_EXTERNAL_HV_VOLTAGE_MISSING);
            return true;
        }
        return false;
    }

    if (!isPackVoltageValid())
    {
        if (elapsed > pack_timeout_ms)
        {
            enterFaultState(DTC_COM_PACK_VOLTAGE_MISSING);
            return true;
        }
        return false;
    }

    const float pack_voltage = getPackVoltage();
    const float hv_voltage = getHvBusVoltage();
    const float tolerance = std::fabs(_voltageMatchTolerance_v);
    const float delta = std::fabs(pack_voltage - hv_voltage);

    if (delta <= tolerance)
    {
        _positiveContactor.close();
        return false;
    }

    if (elapsed > timeout_ms)
    {
        enterFaultState(DTC_COM_PRECHARGE_VOLTAGE_TIMEOUT);
        return true;
    }

    return false;
}

void Contactormanager::setFeedbackDisabled(bool disabled)
{
    if (_feedbackDisabled == disabled)
    {
        return;
    }

    _feedbackDisabled = disabled;

    _prechargeContactor.setFeedbackDisabled(disabled);
    _positiveContactor.setFeedbackDisabled(disabled);

    if (_feedbackDisabled)
    {
        _dtc = DTC_COM_NONE;
        const bool positive_on = _positiveContactor.getOutputPin();
        const bool precharge_on = _prechargeContactor.getOutputPin();

        if (positive_on && precharge_on)
        {
            _currentState = CLOSED;
            _targetState = CLOSED;
        }
        else if (!positive_on && !precharge_on)
        {
            _currentState = OPEN;
            _targetState = OPEN;
        }
        else if (precharge_on && !positive_on)
        {
            _currentState = CLOSING_POSITIVE;
            _targetState = CLOSED;
        }
        else
        {
            _currentState = OPENING_PRECHARGE;
            _targetState = OPEN;
        }
    }
    else
    {
        _dtc = DTC_COM_NONE;
        _targetState = OPEN;
        _currentState = OPEN;
    }
}

bool Contactormanager::isFeedbackDisabled() const
{
    return _feedbackDisabled;
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
        if (_dtc & DTC_COM_PRECHARGE_VOLTAGE_TIMEOUT)
        {
            errorString += "DTC_COM_PRECHARGE_VOLTAGE_TIMEOUT, ";
            hasError = true;
        }
        if (_dtc & DTC_COM_EXTERNAL_HV_VOLTAGE_MISSING)
        {
            errorString += "DTC_COM_EXTERNAL_HV_VOLTAGE_MISSING, ";
            hasError = true;
        }
        if (_dtc & DTC_COM_PACK_VOLTAGE_MISSING)
        {
            errorString += "DTC_COM_PACK_VOLTAGE_MISSING, ";
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

Contactor::DTC_CON Contactormanager::getPositiveDTC() const
{
    return _positiveContactor.getDTC();
}

bool Contactormanager::getPositiveInputPin() const
{
    return _positiveContactor.getInputPin();
}

Contactor::State Contactormanager::getPrechargeState() const
{
    return _prechargeContactor.getState();
}

Contactor::DTC_CON Contactormanager::getPrechargeDTC() const
{
    return _prechargeContactor.getDTC();
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

void Contactormanager::setHvBusVoltage(float voltage_v)
{
    _hvBusVoltage_v = voltage_v;
    _hvBusVoltage_valid = true;
    _hvBusVoltage_lastUpdateMs = millis();
}

float Contactormanager::getHvBusVoltage() const
{
    return _hvBusVoltage_v;
}

bool Contactormanager::isHvBusVoltageValid() const
{
    if (!_hvBusVoltage_valid)
    {
        return false;
    }

    return (millis() - _hvBusVoltage_lastUpdateMs) <= static_cast<unsigned long>(BMS_VCU_TIMEOUT);
}

void Contactormanager::invalidateHvBusVoltage()
{
    _hvBusVoltage_valid = false;
}

void Contactormanager::setPackVoltage(float voltage_v, bool valid)
{
    if (valid && std::isfinite(voltage_v))
    {
        _packVoltage_v = voltage_v;
        _packVoltage_valid = true;
        _packVoltage_lastUpdateMs = millis();
    }
    else
    {
        _packVoltage_valid = false;
    }
}

float Contactormanager::getPackVoltage() const
{
    return _packVoltage_v;
}

bool Contactormanager::isPackVoltageValid() const
{
    if (!_packVoltage_valid)
    {
        return false;
    }

    return (millis() - _packVoltage_lastUpdateMs) <=
           static_cast<unsigned long>(CONTACTOR_PACK_VOLTAGE_VALIDITY_MS);
}

void Contactormanager::setPrechargeStrategy(PrechargeStrategy strategy)
{
    switch (strategy)
    {
    case PrechargeStrategy::TIMED_DELAY:
    case PrechargeStrategy::VOLTAGE_MATCH:
        _prechargeStrategy = strategy;
        break;
    default:
        _prechargeStrategy = PrechargeStrategy::TIMED_DELAY;
        break;
    }
}

Contactormanager::PrechargeStrategy Contactormanager::getPrechargeStrategy() const
{
    return _prechargeStrategy;
}

void Contactormanager::setVoltageMatchTolerance(float tolerance_v)
{
    if (!std::isfinite(tolerance_v))
    {
        _voltageMatchTolerance_v = CONTACTOR_PRECHARGE_VOLTAGE_TOLERANCE;
        return;
    }

    if (tolerance_v < 0.0f)
    {
        tolerance_v = std::fabs(tolerance_v);
    }

    _voltageMatchTolerance_v = tolerance_v;
}

float Contactormanager::getVoltageMatchTolerance() const
{
    return _voltageMatchTolerance_v;
}

void Contactormanager::setVoltageMatchTimeout(uint32_t timeout_ms)
{
    if (timeout_ms == 0U)
    {
        _voltageMatchTimeout_ms = CONTACTOR_PRECHARGE_MATCH_TIMEOUT_MS;
        return;
    }

    _voltageMatchTimeout_ms = timeout_ms;
}

uint32_t Contactormanager::getVoltageMatchTimeout() const
{
    return _voltageMatchTimeout_ms;
}
