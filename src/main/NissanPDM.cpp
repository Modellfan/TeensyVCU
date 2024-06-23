#include <ACAN_T4.h>
#include <Arduino.h>

#include "utils/can_packer.h"
#include "settings.h"
#include <functional> // Add this line to include the <functional> header
#include "NissanPDM.h"

NissanPDM::NissanPDM()
{
    _OBCwake = false; // Monitor if the OBC is awake
    _plugInserted = false;
}

void NissanPDM::initialize()
{
    _state = INIT;
    _dtc = DTC_PDM_NONE;

    _OBCPowerSetpoint = 0.0; // Calculated by following values

    _batteryCurrent = 0.0;
    _batteryVoltage = 290.4;

    _batteryCurrentSetpoint = 40.0;
    _batteryVoltageSetpoint = 401.0;
    _allowChargingBMS = true;

    _maxAcPowerSetpoint = 2000.0;
    _activateCharging = true;

    // Set up CAN port
    ACAN_T4_Settings settings(500 * 1000); // 125 kbit/s
    const uint32_t errorCode = ACAN_T4::PDM_CAN.begin(settings);

#ifdef DEBUG
    Serial.print("Bitrate prescaler: ");
    Serial.println(settings.mBitRatePrescaler);
    Serial.print("Propagation Segment: ");
    Serial.println(settings.mPropagationSegment);
    Serial.print("Phase segment 1: ");
    Serial.println(settings.mPhaseSegment1);
    Serial.print("Phase segment 2: ");
    Serial.println(settings.mPhaseSegment2);
    Serial.print("RJW: ");
    Serial.println(settings.mRJW);
    Serial.print("Triple Sampling: ");
    Serial.println(settings.mTripleSampling ? "yes" : "no");
    Serial.print("Actual bitrate: ");
    Serial.print(settings.actualBitRate());
    Serial.println(" bit/s");
    Serial.print("Exact bitrate ? ");
    Serial.println(settings.exactBitRate() ? "yes" : "no");
    Serial.print("Distance from wished bitrate: ");
    Serial.print(settings.ppmFromWishedBitRate());
    Serial.println(" ppm");
    Serial.print("Sample point: ");
    Serial.print(settings.samplePointFromBitStart());
    Serial.println("%");
#endif

    if (0 == errorCode)
    {
        Serial.println("PDM CAN ok");
    }
    else
    {
        Serial.print("Error PDM CAN: 0x");
        Serial.println(errorCode, HEX);
        _dtc |= DTC_PDM_CAN_INIT_ERROR;
    }
}

void NissanPDM::print()
{
    Serial.printf("PDM: Awake %d, Charge Status %d, Plug inserted %d, AC Voltage %d, Charge Power %f\n", _OBCwake, _OBC_Charge_Status, _plugInserted, _OBC_Status_AC_Voltage, _OBC_Charge_Power);
    Serial.printf("     Battery Voltage %f, Battery Current %f, Voltage Setpoint %f, Current Setpoint %f, Allow Charging %d\n", _batteryVoltage, _batteryCurrent, _batteryVoltageSetpoint, _batteryCurrentSetpoint, _allowChargingBMS);
    Serial.printf("     AC Power Setpoint %f, Activate Charging %d, OBC Power Setpoint %f, Limit Reason %s\n", _maxAcPowerSetpoint, _activateCharging, _OBCPowerSetpoint, this->getChargeLimitingReasonString());
    Serial.printf("     State %s, DTC %s\n", this->getStateString(), this->getDTCString().c_str());
    Serial.println();
}

// Helper to send CAN message
void NissanPDM::send_message(CANMessage *frame)
{
    if (ACAN_T4::PDM_CAN.tryToSend(*frame))
    {
        // Serial.println("Send ok");
    }
    else
    {
        _dtc |= DTC_PDM_CAN_SEND_ERROR;
    }
}

const char *NissanPDM::getStateString()
{
    switch (_state)
    {
    case INIT:
        return "INIT";
    case OPERATING:
        return "OPERATING";
    case FAULT:
        return "FAULT";
    default:
        return "UNKNOWN STATE";
    }
}

String NissanPDM::getDTCString()
{
    String errorString = "";

    if (static_cast<uint8_t>(_dtc) == 0)
    {
        errorString += "None";
    }
    else
    {
        if (_dtc & DTC_PDM_CAN_SEND_ERROR)
        {
            errorString += "DTC_PDM_CAN_SEND_ERROR, ";
        }
        if (_dtc & DTC_PDM_CAN_INIT_ERROR)
        {
            errorString += "DTC_PDM_CAN_INIT_ERROR, ";
        }
        if (_dtc & DTC_PDM_MODULE_FAULT)
        {
            errorString += "DTC_PDM_MODULE_FAULT, ";
        }

        // Remove the trailing comma and space
        errorString.remove(errorString.length() - 2);
    }
    return errorString;
}

const char *NissanPDM::getChargeLimitingReasonString()
{
    switch (_limitReason)
    {
    case LIMITED_BY_HARDWARE:
        return "LIMITED BY HARDWARE";
    case PLUG_NOT_CONNECTED:
        return "PLUG NOT CONNECTED";
    case CHARGER_NOT_ENABLED:
        return "CHARGER NOT ENABLED";
    case AC_POWER_LIMIT:
        return "AC POWER LIMIT";
    case DC_CURRENT_LIMIT:
        return "DC CURRENT LIMIT";
    case BATTERY_VOLTAGE_LEVEL:
        return "BATTERY VOLTAGE LEVEL";
    case BMS_CHARGING_NOT_ALLOWED:
        return "BMS CHARGING NOT ALLOWED";
    default:
        return "UNKNOWN REASON";
    }
}

/*Info on running Leaf Gen 2,3 PDM
IDs required :
0x1D4
0x1DB
0x1DC
0x1F2
0x50B
0x55B
0x59E
0x5BC

PDM sends:
0x390
0x393
0x679 on evse plug insert

For QC:

From PDM to QC

PDM EV CAN --------to-------QC CAN
0x3b9                       0x100
0x3bb                       0x101
0x3bc                       0x102
0x3be                       0x200
0x4ba                       0x110
0x4bb                       0x201
0x4bc                       0x700
0x4c0                       0x202

From QC to PDM

QC CAN----------------------PDM EV CAN
0x108                       0x3c8
0x109                       0x3c9
0x208                       0x3cd
0x209                       0x4be


*/

void NissanPDM::Task2Ms()
{
    // ToDo Implement timeout here

    CANMessage msg;

    if (ACAN_T4::PDM_CAN.receive(msg))
    {
        switch (msg.id)
        {
        case 0x679:          // This message fires once when charging cable is plugged in
            _OBCwake = true; // 0x679 is received once when we plug in if pdm is asleep so wake wakey...
            break;

        //   BO_ 912 x390: 8 OBCpd
        //   SG_ OBC_Status_AC_Voltage : 27|2@1+ (1,0) [0|3] "status" Vector__XXX
        //   SG_ OBC_Flag_QC_Relay_On_Announcemen : 38|1@1+ (1,0) [0|1] "status" Vector__XXX
        //   SG_ OBC_Flag_QC_IR_Sensor : 47|1@1+ (1,0) [0|1] "status" Vector__XXX
        //   SG_ OBC_Maximum_Charge_Power_Out : 40|9@0+ (0.1,0) [0|50] "kW" Vector__XXX
        //   SG_ PRUN_390 : 60|2@1+ (1,0) [0|3] "PRUN" Vector__XXX
        //   SG_ OBC_Charge_Status : 41|6@1+ (1,0) [0|0] "status" Vector__XXX
        //   SG_ CSUM_390 : 56|4@1+ (1,0) [0|16] "CSUM" Vector__XXX
        //   SG_ OBC_Charge_Power : 0|9@0+ (0.1,0) [0|50] "kW" Vector__XXX
        case 0x390:
            // VAL_ 912 OBC_Charge_Status 1 "Idle OR QC" 2 "Finished" 4 "Charging OR interrupted" 8 "Idle" 9 "Idle" 12 "Plugged in waiting on timer" ;
            _OBC_Charge_Status = ((msg.data[5] & 0x7E) >> 1);
            if (_OBC_Charge_Status == 12 || 4)
            {
                _plugInserted = true; // plug inserted
            }
            else
            {
                _plugInserted = false; // plug not inserted
            }

            // VAL_ 912 OBC_Status_AC_Voltage 0 "No Signal" 1 "100V" 2 "200V" 3 "Abnormal Wave" ;
            _OBC_Status_AC_Voltage = ((msg.data[3] & 0x18) >> 3);

            // CM_ SG_ 912 OBC_Charge_Power "Actual charger output - From OVMS code";
            _OBC_Charge_Power = (((msg.data[0] & 0x01) << 8) | (msg.data[1])) * 0.1;

            break;
        default:
            break;
        }
    }
}

void NissanPDM::Task10Ms()
{
    CANMessage msg;

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // BO_ 468 x1D4: 8 VCM
    //  SG_ MotorAmpTorqueRequest : 23|12@0- (0.25,0) [0|1024] "Nm" Vector__XXX
    //  SG_ HCM_CLOCK : 38|2@1+ (1,0) [0|3] "-" Vector__XXX
    //  SG_ StatusOfHighVoltagePowerSupply : 34|1@1+ (1,0) [0|0] "-" Vector__XXX
    //  SG_ Relay_Plus_Output_Status : 46|1@1+ (1,0) [0|0] "-" Vector__XXX
    //  SG_ CRC_1D4 : 56|8@1+ (1,0) [0|255] "" Vector__XXX
    //  SG_ ChargeStatus : 48|8@1+ (1,0) [0|255] "MODEMASK" Vector__XXX
    /////////////////////////////////////////////////////////////////////////////////////////////////

    msg.id = 0x1D4;
    msg.len = 8;
    msg.data[0] = 0x6E;
    msg.data[1] = 0x6E;
    msg.data[2] = 0x00;
    msg.data[3] = 0x00;
    msg.data[4] = 0x02 | (_counter_1d4 << 6);
    ;
    msg.data[5] = 0x06;
    msg.data[6] = 0xE0;
    // Extra CRC in byte 7
    nissan_crc(msg.data, 0x85);

    _counter_1d4++;
    if (_counter_1d4 >= 4)
        _counter_1d4 = 0;

    send_message(&msg);

    // Byte 4 could have variations
    // Peter
    //  byte message1d4_0[8] = {0x6E, 0x6E, 0x00, 0x00, 0x42, 0x06, 0xE0, 0x0D};
    //  byte message1d4_1[8] = {0x6E, 0x6E, 0x00, 0x00, 0xb2, 0x06, 0xE0, 0xC1};
    //  byte message1d4_2[8] = {0x6E, 0x6E, 0x00, 0x00, 0xC2, 0x06, 0xE0, 0xD6};
    //  byte message1d4_3[8] = {0x6E, 0x6E, 0x00, 0x00, 0x02, 0x06, 0xE0, 0xCA};

    // Damien
    //  MSB nibble: Runs through the sequence 0, 4, 8, C
    // LSB nibble: Precharge report (precedes actual precharge
    //             control)
    //   0: Discharging (5%)
    //   2: Precharge not started (1.4%)
    //   3: Precharging (0.4%)
    //   5: Starting discharge (3x10ms) (2.0%)
    //   7: Precharged (93%)
    // bytes[4] = 0x07 | (counter_1d4 << 6);
    // bytes[4] = 0x02 | (counter_1d4 << 6);
    // Bit 2 is HV status. 0x00 No HV, 0x01 HV On.

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x1DB
    // BO_ 475 x1DB: 8 HVBAT
    // SG_ LB_Current : 7|11@0- (0.5,0) [-400|200] "A" Vector__XXX
    // SG_ LB_Relay_Cut_Request : 11|2@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //    0x00 no req
    //    0x01,0x02,0x03 main relay off req
    // SG_ LB_Failsafe_Status : 8|3@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //    0x00 Normal start req. seems to stay on this value most of the time
    //    0x01 Normal stop req
    //    0x02 Charge stop req
    //    0x03 Charge and normal stop req. Other values call for a caution lamp which we don't need
    // SG_ LB_Total_Voltage : 23|10@0+ (0.5,0) [0|450] "V" Vector__XXX
    // SG_ LB_MainRelayOn_flag : 29|1@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    // SG_ LB_Full_CHARGE_flag : 28|1@1+ (1,0) [0|0] "" Vector__XXX
    // SG_ LB_INTER_LOCK : 27|1@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    // SG_ LB_Discharge_Power_Status : 25|2@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    // SG_ LB_Voltage_Latch_Flag : 24|1@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    // SG_ LB_Usable_SOC : 32|7@1+ (1,0) [0|0] "" Vector__XXX
    // SG_ LB_PRUN_1DB : 48|2@1+ (1,0) [0|0] "" Vector__XXX
    // SG_ CRC_1DB : 56|8@1+ (1,0) [0|0] "CRC" Vector__XXX
    /////////////////////////////////////////////////////////////////////////////////////////////////

    int16_t _batteryCurrentInt = static_cast<int16_t>(_batteryCurrent * 2);
    int16_t _batteryVoltageInt = static_cast<int16_t>(_batteryVoltage * 2);

    // msg.id = 0x1DB;
    // msg.len = 8;
    // msg.data[0] = _batteryCurrentInt >> 3;
    // msg.data[1] = _batteryCurrentInt & 0xE0;
    // msg.data[2] = _batteryVoltageInt >> 2;
    // msg.data[3] = ((_batteryVoltageInt & 0xC0) | (0x2b)); // 0x2b should give no cut req, main rly on permission,normal p limit.
    // msg.data[4] = 0x40;
    // msg.data[5] = 0x00;
    // msg.data[6] = _counter_1db;
    // // Extra CRC in byte 7
    // nissan_crc(msg.data, 0x85);

    msg.id = 0x1DB;
    msg.len = 8;
    msg.data[0] = 0x00;
    msg.data[1] = 0x00;
    msg.data[2] = 0x00;
    msg.data[3] = 0x00;
    msg.data[4] = 0x40;
    msg.data[5] = 0x00;
    msg.data[6] = _counter_1db;
    // Extra CRC in byte 7
    nissan_crc(msg.data, 0x85);

    _counter_1db++;
    if (_counter_1db >= 4)
        _counter_1db = 0;

    send_message(&msg);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x50B - 10ms
    //  BO_ 1291 x50B: 7 VCM
    //  SG_ DiagMuxOn_VCM : 18|1@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ HCM_WakeUpSleepCmd : 30|2@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ Batt_Heater_Mail_Send_OK : 53|1@1+ (1,0) [0|0] "" Vector__XXX
    /////////////////////////////////////////////////////////////////////////////////////////////////

    // Statistics from 2016 capture:
    //     10 00000000000000
    //     21 000002c0000000
    //    122 000000c0000000
    //    513 000006c0000000

    // Let's just send the most common one all the time
    // FIXME: This is a very sloppy implementation. Thanks. I try:)
    msg.id = 0x50B;
    msg.len = 7;
    msg.data[0] = 0x00;
    msg.data[1] = 0x00;
    msg.data[2] = 0x06;
    msg.data[3] = 0xc0;
    msg.data[4] = 0x00;
    msg.data[5] = 0x00;
    msg.data[6] = 0x00;

    send_message(&msg);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x1DC:
    // BO_ 476 x1DC: 8 HVBAT
    //  SG_ LB_Discharge_Power_Limit : 7|10@0+ (0.25,0) [0|254] "kW" Vector__XXX
    //  SG_ LB_Charge_Power_Limit : 13|10@0+ (0.25,0) [0|254] "kW" Vector__XXX
    //  SG_ LB_MAX_POWER_FOR_CHARGER : 19|10@0+ (0.1,-10) [-10|90] "kW" Vector__XXX
    //  SG_ LB_Charge_Power_Status : 24|2@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_BPCMAX_UPRATE : 37|3@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_CODE_CONDITION : 34|3@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ LB_CODE1 : 33|8@0+ (1,0) [0|0] "" Vector__XXX
    //  SG_ LB_CODE2 : 41|8@0+ (1,0) [0|0] "" Vector__XXX
    //  SG_ LB_PRUN_1DC : 48|2@1+ (1,0) [0|3] "" Vector__XXX
    //  SG_ CRC_1DC : 56|8@1+ (1,0) [0|0] "CRC" Vector__XXX
    // 0x1dc from lbc. Contains chg power lims and disch power lims.
    // Disch power lim in byte 0 and byte 1 bits 6-7. Just set to max for now.
    // Max charging power in bits 13-20. 10 bit unsigned scale 0.25.Byte 1 limit in kw.
    /////////////////////////////////////////////////////////////////////////////////////////////////

    msg.id = 0x1DC;
    msg.len = 8;
    msg.data[0] = 0x6E;
    msg.data[1] = 0x0A;
    msg.data[2] = 0x05;
    msg.data[3] = 0xD5;
    msg.data[4] = 0x00;
    msg.data[5] = 0x00;
    msg.data[6] = _counter_1dc;
    // Extra CRC in byte 7
    nissan_crc(msg.data, 0x85);

    _counter_1dc++;
    if (_counter_1dc >= 4)
        _counter_1dc = 0;
    send_message(&msg);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x1F2: Charge Power and DC/DC Converter Control
    //  BO_ 498 x1F2: 8 VCM
    //  SG_ CommandedChargePower : 1|10@0+ (1,0) [0|0] "" Vector__XXX
    //  SG_ TargetCharge_SOC : 7|1@1+ (1,0) [0|0] "modemask" Vector__XXX
    //  SG_ Charge_StatusTransitionReqest : 21|2@1+ (1,0) [0|3] "modemask" Vector__XXX
    //  SG_ Keep_SOC_Request : 20|1@1+ (1,0) [0|0] "modemask" Vector__XXX
    //  SG_ PCS_Connector_Detection : 17|1@1+ (1,0) [0|0] "modemask" Vector__XXX
    //  SG_ MPRUN : 48|2@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ Unknown_498 : 63|1@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ CSUM_498 : 56|4@1+ (1,0) [0|0] "" Vector__XXX

    // convert power setpoint to PDM format:
    //    0xA0 = 15A (60x) =3500W
    //    0x70 = 3 amps ish (12x)
    //    0x6a = 1.4A (6x)
    //    0x66 = 0.5A (2x)
    //    0x65 = 0.3A (1x)
    //    0x64 = no chg = 0W
    //    so 0x64=100. 0xA0=160. so 60 decimal steps. 1 step=100W???
    //    3500W = 60 -> 58
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // This line controls if power should flow or not
    _OBCPowerSetpoint = 10000; // Start with a high initial value. Actually 6.6kw
    _limitReason = LIMITED_BY_HARDWARE;

    // Plug is not inserted
    if (!_plugInserted)
    {
        _limitReason = PLUG_NOT_CONNECTED;
        _OBCPowerSetpoint = 0;
    }

    // User deactivated charging in UI
    if (!_activateCharging)
    {
        _limitReason = CHARGER_NOT_ENABLED;
        _OBCPowerSetpoint = 0;
    }

    // BMS does not allow for charging
    if (!_allowChargingBMS)
    {
        _limitReason = BMS_CHARGING_NOT_ALLOWED;
        _OBCPowerSetpoint = 0;
    }

    // Current Setpoint is limiting reason
    float bmsPowerSetpoint = _batteryVoltage * _batteryCurrentSetpoint;
    if (bmsPowerSetpoint < _OBCPowerSetpoint)
    {
        _OBCPowerSetpoint = bmsPowerSetpoint;
        _limitReason = DC_CURRENT_LIMIT;
    }

    // AC Power Limit in User Interface
    if (_maxAcPowerSetpoint < _OBCPowerSetpoint)
    {
        _OBCPowerSetpoint = _maxAcPowerSetpoint;
        _limitReason = AC_POWER_LIMIT;
    }

    // FOr additional safety check for voltage level. This should never kick, because BMS should trottle current before reaching max voltage
    if (_batteryVoltage >= _batteryVoltageSetpoint)
    {
        _OBCPowerSetpoint = 0;
        _limitReason = BATTERY_VOLTAGE_LEVEL;
    }

    // Convert power setpoint to PDM scale and clamp to min and max value
    u_int8_t _OBCPowerSetpointInt = _OBCPowerSetpoint / 58 + 0x64;
    if (_OBCPowerSetpointInt > 0xA0)
    { // 15A TODO, raise once cofirmed how to map bits into frame0 and frame1
        _OBCPowerSetpointInt = 0xA0;
    }
    else if (_OBCPowerSetpointInt <= 0x64)
    {
        _OBCPowerSetpointInt = 0x64; // 100W? stuck at 100 in drive mode (no charging)
    }

    // Commanded chg power in byte 1 and byte 0 bits 0-1. 10 bit number.
    // byte 1=0x64 and byte 0=0x00 at 0 power.
    // 0x00 chg 0ff dcdc on.
    msg.id = 0x1F2;
    msg.len = 8;
    msg.data[0] = 0x30;
    msg.data[1] = 0xA0; //_OBCPowerSetpointInt;
    msg.data[2] = 0x20;
    msg.data[3] = 0xAC;
    msg.data[4] = 0x00;
    msg.data[5] = 0x3C;
    msg.data[6] = _counter_1f2;
    // Extra CRC in byte 7
    nissan_crc(msg.data, 0x85);

    _counter_1f2++;
    if (_counter_1f2 >= 4)
        _counter_1f2 = 0;
    send_message(&msg);
}

void NissanPDM::Task100Ms()
{
    CANMessage msg;

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x55B - 100ms
    //  BO_ 1371 x55B: 8 HVBAT
    //  SG_ LB_SOC : 7|10@0+ (1,0) [0|1000] "%+1" Vector__XXX
    //  SG_ LB_ALU_ANSWER : 16|8@1+ (1,0) [85|170] "" Vector__XXX
    //  SG_ LB_IR_Sensor_Wave_Voltage : 39|10@0+ (1,0) [0|4990] "mV (5000/1024)" Vector__XXX
    //  SG_ LB_IR_Sensor_Malfunction : 40|1@1+ (1,0) [0|1] "modemask" Vector__XXX
    //  SG_ LB_Capacity_Empty : 55|1@1+ (1,0) [0|1] "modemask" Vector__XXX
    //  SG_ LB_RefusetoSleep : 53|2@1+ (1,0) [0|3] "modemask" Vector__XXX
    //  SG_ LB_PRUN_55B : 48|2@1+ (1,0) [0|3] "" Vector__XXX
    //  SG_ CRC_55B : 56|8@1+ (1,0) [0|0] "CRC" Vector__XXX
    /////////////////////////////////////////////////////////////////////////////////////////////////

    msg.id = 0x55b;
    msg.len = 8;
    msg.data[0] = 0xA4;
    msg.data[1] = 0x40;
    msg.data[2] = 0xAA;
    msg.data[3] = 0x00;
    msg.data[4] = 0xDF;
    msg.data[5] = 0xC0;
    msg.data[6] = ((0x1 << 4) | (_counter_55b));
    // Extra CRC in byte 7
    nissan_crc(msg.data, 0x85);

    _counter_55b++;
    if (_counter_55b >= 4)
        _counter_55b = 0;
    send_message(&msg);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x59E:
    //  BO_ 1438 x59E: 8 HVBAT
    //  SG_ LB_Full_Capacity_for_QC : 20|9@0+ (100,0) [0|50000] "Wh" Vector__XXX
    //  SG_ LB_Remain_Capacity_for_QC : 27|9@0+ (100,0) [0|50000] "Wh" Vector__XXX
    /////////////////////////////////////////////////////////////////////////////////////////////////

    msg.id = 0x59e;
    msg.len = 8;
    msg.data[0] = 0x00; // Static msg works fine here
    msg.data[1] = 0x00; // Batt capacity for chg and qc.
    msg.data[2] = 0x0c;
    msg.data[3] = 0x76;
    msg.data[4] = 0x18;
    msg.data[5] = 0x00;
    msg.data[6] = 0x00;
    msg.data[7] = 0x00;

    send_message(&msg);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x5BC:
    //  BO_ 1468 x5BC: 8 HVBAT
    //  SG_ LB_Remain_Capacity_GIDS : 7|10@0+ (1,0) [0|500] "gids" Vector__XXX
    //  SG_ LB_Remaining_Capacity_Segments : 16|8@1+ (1,0) [0|240] "" Vector__XXX
    //  SG_ LB_Temperature_Segment_For_Dash : 24|8@1+ (0.4166666,0) [0|100] "%" Vector__XXX
    //  SG_ LB_Capacity_Deterioration_Rate : 33|7@1+ (1,0) [0|100] "%" Vector__XXX
    //  SG_ LB_Remain_Cap_Segment_Swit_Flag : 32|1@1+ (1,0) [0|1] "status" Vector__XXX
    //  SG_ LB_Output_Power_Limit_Reason : 45|3@1+ (1,0) [0|7] "modemask" Vector__XXX
    //  SG_ LB_Remain_Charge_Time_Condition : 41|5@0+ (1,0) [0|30] "modemask" Vector__XXX
    //  SG_ LB_Remain_Charge_Time : 52|13@0+ (1,0) [0|8190] "minutes" Vector__XXX
    //  SG_ Mux_5BC M : 32|4@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ ChargeBars m8 : 20|4@1+ (1,0) [0|15] "-" Vector__XXX
    //  SG_ CapacityBars m9 : 20|4@1+ (1,0) [0|15] "-" Vector__XXX
    //  SG_ LB_MaxGIDS : 44|1@1+ (1,0) [0|0] "" Vector__XXX
    /////////////////////////////////////////////////////////////////////////////////////////////////

    // muxed msg with info for gids etc. Will try static for a test.
    msg.id = 0x5bc;
    msg.len = 8;
    msg.data[0] = 0x3D; // Static msg works fine here
    msg.data[1] = 0x80;
    msg.data[2] = 0xF0;
    msg.data[3] = 0x64;
    msg.data[4] = 0xB0;
    msg.data[5] = 0x01;
    msg.data[6] = 0x00;
    msg.data[7] = 0x32;

    send_message(&msg);
}

void NissanPDM::Monitor100Ms()
{
}

void NissanPDM::Monitor1000Ms()
{
    print();
}

int8_t NissanPDM::fahrenheit_to_celsius(uint16_t fahrenheit)
{
    int16_t result = ((int16_t)fahrenheit - 32) * 5 / 9;
    if (result < -128)
        return -128;
    if (result > 127)
        return 127;
    return result;
}

void NissanPDM::nissan_crc(uint8_t *data, uint8_t polynomial)
{
    // We want to process 8 bytes with the 8th byte being zero
    data[7] = 0;
    uint8_t crc = 0;
    for (int b = 0; b < 8; b++)
    {
        for (int i = 7; i >= 0; i--)
        {
            uint8_t bit = ((data[b] & (1 << i)) > 0) ? 1 : 0;
            if (crc >= 0x80)
                crc = (uint8_t)(((crc << 1) + bit) ^ polynomial);
            else
                crc = (uint8_t)((crc << 1) + bit);
        }
    }
    data[7] = crc;
}
