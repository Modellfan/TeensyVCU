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
    String getDTCString();

    bool _OBCwake;                  // Monitor if the OBC is awake
    bool _plugInserted;             // Monitor if the plug is inserted
    uint8_t _OBC_Charge_Status;     // Status of OBC
    uint8_t _OBC_Status_AC_Voltage; // Status of applied AC voltage
    float _OBC_Charge_Power;        // Current charge power

    float _batteryCurrent;
    float _batteryVoltage;
    float _maxChargeCurrent;
    float _targetVoltageSetpoint;

    uint8_t _counter_55b; // Framecounter for PRUN
    uint8_t _counter_1db = 0;
    uint8_t _counter_1dc = 0;
    uint8_t _counter_1f2 = 0;

    STATE_PDM _state;
    DTC_PDM _dtc;
};

static uint16_t Vbatt = 0;
static uint16_t VbattSP = 0;
static uint8_t counter_1db = 0;
static uint8_t counter_1dc = 0;
static uint8_t counter_1f2 = 0;
static uint8_t counter_55b = 0;
static uint8_t OBCpwrSP = 0;
static uint8_t OBCpwr = 0;
static bool OBCwake = false;
static bool PPStat = false;
static uint8_t OBCVoltStat = 0;
static uint8_t PlugStat = 0;
static uint16_t calcBMSpwr = 0;

#endif // NISSANPDM_H
