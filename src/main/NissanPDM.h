#ifndef PDM_H
#define PDM_H

#include <ACAN_T4.h>
#include <Arduino.h>
#include "settings.h"
#include <functional> // Add this line to include the <functional> header

class NissanPDM
{
public:
    enum STATE_PDM
    {
        INIT,      // pack is being initialized
        OPERATING, // pack is in operation state
        FAULT      // pack is in fault state
    };

    typedef enum
    {
        DTC_PDM_NONE = 0,
        DTC_PDM_CAN_SEND_ERROR = 1 << 0,
        DTC_PDM_CAN_INIT_ERROR = 1 << 1,
        DTC_PDM_MODULE_FAULT = 1 << 2,
    } DTC_PDM;

    typedef enum
    {
        LIMITED_BY_HARDWARE = 0,
        PLUG_NOT_CONNECTED = 1 << 0,
        CHARGER_NOT_ENABLED = 1 << 1,
        AC_POWER_LIMIT = 1 << 2,
        DC_CURRENT_LIMIT = 1 << 3,
        BATTERY_VOLTAGE_LEVEL = 1 << 4,
        BMS_CHARGING_NOT_ALLOWED = 1 << 5,
    } CHARGE_CURRENT_LIMITING_REASON_PDM;

    //Constructor
    NissanPDM();

    // Runnables
    void initialize();
    void Task2Ms();
    void Task10Ms();
    void Task100Ms();

    void Monitor100Ms();
    void Monitor1000Ms();
    void print();

    // Setter and getter functions
    bool ControlCharge(bool RunCh, bool ACReq);

private:
    // Helper functions
    static void nissan_crc(uint8_t *data, uint8_t polynomial);
    static int8_t fahrenheit_to_celsius(uint16_t fahrenheit);
    void send_message(CANMessage *frame); // Send out CAN message

    const char *getStateString();
    const char *getChargeLimitingReasonString();
    String getDTCString();
    
    //Charger status signals
    bool _OBCwake;                  // Monitor if the OBC is awake
    bool _plugInserted;             // Monitor if the plug is inserted
    uint8_t _OBC_Charge_Status;     // Status of OBC
    uint8_t _OBC_Status_AC_Voltage; // Status of applied AC voltage
    float _OBC_Charge_Power;        // Current charge power

    //External control signals
    float _OBCPowerSetpoint = 0.0; //Calculated by following values

    float _batteryCurrent = 0.0; //from BMS
    float _batteryVoltage = 0.0; //from BMS

    float _batteryCurrentSetpoint= 0.0; //from BMS
    float _batteryVoltageSetpoint= 0.0; //from BMS
    bool _allowChargingBMS = false;    //Only charging, when BMS allows it

    float _maxAcPowerSetpoint = 0.0; //from User Interface. Protect the socket.
    bool _activateCharging = false;    //from User Interface
    
    uint8_t _counter_55b = 0; // Framecounter for PRUN
    uint8_t _counter_1db = 0;
    uint8_t _counter_1dc = 0;
    uint8_t _counter_1f2 = 0;
    uint8_t _counter_1d4 = 0;

    CHARGE_CURRENT_LIMITING_REASON_PDM _limitReason;
    STATE_PDM _state;
    DTC_PDM _dtc;
};

#endif // NISSANPDM_H
