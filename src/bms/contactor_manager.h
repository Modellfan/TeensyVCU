#ifndef CONTACTORMANAGER_H
#define CONTACTORMANAGER_H

#include <Arduino.h>
#include "settings.h"
#include "bms/contactor.h"
#include <ACAN_T4.h>
#include <functional> // Add this line to include the <functional> header

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
    } DTC_COM;

    Contactormanager();

    void initialise();
    void close();
    void open();
    State getState();
    DTC_COM getDTC();
    void update();
    void setFeedbackDisabled(bool disabled);
    bool isFeedbackDisabled() const;
    Contactor::State getPositiveState() const;
    Contactor::DTC_CON getPositiveDTC() const;
    bool getPositiveInputPin() const;
    Contactor::State getPrechargeState() const;
    Contactor::DTC_CON getPrechargeDTC() const;
    bool getPrechargeInputPin() const;
    bool isNegativeContactorClosed() const;
    bool isContactorVoltageAvailable() const;
    void setHvBusVoltage(float voltage_v);
    float getHvBusVoltage() const;
    bool isHvBusVoltageValid() const;
    void invalidateHvBusVoltage();

private:
    const char *getCurrentStateString();
    const char *getTargetStateString();
    String getDTCString();

    Contactor _prechargeContactor;
    Contactor _positiveContactor;
    unsigned long _lastPreChangeTime;
    State _currentState;
    State _targetState;
    DTC_COM _dtc;

    bool _negativeContactor_closed;
    bool _contactorVoltage_available;
    float _hvBusVoltage_v;
    bool _hvBusVoltage_valid;
    unsigned long _hvBusVoltage_lastUpdateMs;
    bool _feedbackDisabled;
};

#endif // CONTACTORMANAGER_H
