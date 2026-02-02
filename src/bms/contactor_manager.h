#ifndef CONTACTORMANAGER_H
#define CONTACTORMANAGER_H

#include <Arduino.h>
#include "settings.h"
#include "bms/contactor.h"
#include <ACAN_T4.h>
class Contactormanager
{
public:
    enum State
    {
        INIT, // Contactors are being initialized
        OPEN, // Contactors are open

        CLOSING_PRECHARGE, // Precharge contactor is closing
        CLOSING_POSITIVE,  // Positive contactor is closing

        CLOSED, // Contactors are closed

        OPENING_POSITIVE,  // Positive contactor is opening
        OPENING_PRECHARGE, // Precharge contactor is opening

        FAULT // Contactors are in fault state
    };

    typedef enum
    {
        DTC_COM_NONE = 0,
        DTC_COM_NO_CONTACTOR_POWER_SUPPLY = 1 << 0,
        DTC_COM_NEGATIVE_CONTACTOR_FAULT = 1 << 1,
        DTC_COM_PRECHARGE_CONTACTOR_FAULT = 1 << 2,
        DTC_COM_POSITIVE_CONTACTOR_FAULT = 1 << 3,
        DTC_COM_PRECHARGE_VOLTAGE_TIMEOUT = 1 << 4,
        DTC_COM_VOLTAGE_MONITOR_INVALID = 1 << 5,
    } DTC_COM;

    enum class PrechargeStrategy : uint8_t
    {
        TIMED_DELAY = CONTACTOR_PRECHARGE_STRATEGY_TIMED_DELAY,
        VOLTAGE_MATCH = CONTACTOR_PRECHARGE_STRATEGY_VOLTAGE_MATCH
    };

    Contactormanager();

    void initialise();
    void close();
    void open();
    State getState();
    DTC_COM getDTC();
    void update();
    Contactor::State getPositiveState() const;
    Contactor::DTC_CON getPositiveDTC() const;
    bool getPositiveInputPin() const;
    Contactor::State getPrechargeState() const;
    Contactor::DTC_CON getPrechargeDTC() const;
    bool getPrechargeInputPin() const;
    bool isNegativeContactorClosed() const;
    bool isContactorVoltageAvailable() const;
    void setPrechargeStrategy(PrechargeStrategy strategy);
    PrechargeStrategy getPrechargeStrategy() const;

private:
    const char *getCurrentStateString();
    const char *getTargetStateString();
    String getDTCString();
    void enterFaultState(DTC_COM dtc);
    bool handleClosingPositiveTimedDelay();
    bool handleClosingPositiveVoltageMatch();

    Contactor _prechargeContactor;
    Contactor _positiveContactor;
    unsigned long _lastPreChangeTime;
    State _currentState;
    State _targetState;
    DTC_COM _dtc;

    bool _negativeContactor_closed;
    bool _contactorVoltage_available;
    PrechargeStrategy _prechargeStrategy;
};

#endif // CONTACTORMANAGER_H
