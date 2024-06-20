#include <Arduino.h>
#include "settings.h"
#include <ACAN_T4.h>
#include "bms/contactor_manager.h"
#include "bms/contactor.h"
#include "utils/can_packer.h"

Contactormanager::Contactormanager() : _prechargeContactor(CONTACTOR_PRCHG_OUT_PIN, CONTACTOR_PRCHG_IN_PIN, CONTACTOR_DEBOUNCE, CONTACTOR_TIMEOUT),
                                       _positiveContactor(CONTACTOR_POS_OUT_PIN, CONTACTOR_POS_IN_PIN, CONTACTOR_DEBOUNCE, CONTACTOR_TIMEOUT)
{
    _currentState = INIT;
}

void Contactormanager::print()
{
    Serial.println("Contactor manager:");
    Serial.printf("    Pack: %3.2fV; Lowest Cell: %3.2fV; Highest Cell: %3.2fV; Balancing Target: %3.2fV; Balancing Activated: %d; Any Module Balancing %d; State: %s; DTC : %s\n", get_pack_voltage(), get_lowest_cell_voltage(), get_highest_cell_voltage(), balanceTargetVoltage, balanceActive, this->get_any_module_balancing(), this->getStateString(), this->getDTCString().c_str());
    Serial.println("");
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

void Contactormanager::monitor(std::function<void(const CANMessage &)> callback)
{
    CANMessage msg;

    msg.data64 = 0;
    msg.id = 1080; // Message 0x438 contactor_manager_state 8bits None
    msg.len = 8;
    pack(msg, getState(), 0, 8, false, 1, 0);                   // getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, getDTC(), 8, 8, false, 1, 0);                     // getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, _negativeContactor_closed, 16, 1, false);         // negative_contactor_input : 16|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, _positiveContactor.getInputPin(), 17, 1, false);  // positive_contactor_input : 17|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, _prechargeContactor.getInputPin(), 18, 1, false); // precharge_contactor_input : 18|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, _contactorVoltage_available, 19, 1, false);       // contactor_supply_voltage_input : 19|1 little_endian unsigned scale: 1, offset: 0, unit: None, None

    if (callback != nullptr)
    {
        callback(msg);
    }
}

Contactormanager::DTC_COM Contactormanager::getDTC() { return _dtc; }
