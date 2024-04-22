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
    // Serial.printf("Pack: %3.2fV : %dmV : %s : %s\n", voltage, cellDelta, this->getStateString(), this->getDTCString().c_str());
    // for (int m = 0; m < numModules; m++)
    // {
    //     modules[m].print();
    // }
    // Serial.println("");
}

// Helper to send CAN message
void NissanPDM::send_message(CANMessage *frame)
{
    if (ACAN_T4::BATTERY_CAN.tryToSend(*frame))
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
    CANMessage msg;

    if (ACAN_T4::BATTERY_CAN.receive(msg))
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
}

void NissanPDM::Monitor100Ms()
{
}

void NissanPDM::Monitor1000Ms()
{
    print();
}

/* void NissanPDM::Task10Ms()
{
    int opmode = Param::GetInt(Param::opmode);

    uint8_t bytes[8];

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x1DB

    // We need to send 0x1db here with voltage measured by inverter
    // Zero seems to work also on my gen1
    ////////////////////////////////////////////////////////////////
    //    BO_ 475 x1DB: 8 HVBAT
    //  SG_ LB_Current : 7|11@0- (0.5,0) [-400|200] "A" Vector__XXX
    //  SG_ LB_Relay_Cut_Request : 11|2@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_Failsafe_Status : 8|3@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_Total_Voltage : 23|10@0+ (0.5,0) [0|450] "V" Vector__XXX
    //  SG_ LB_MainRelayOn_flag : 29|1@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_Full_CHARGE_flag : 28|1@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ LB_INTER_LOCK : 27|1@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_Discharge_Power_Status : 25|2@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_Voltage_Latch_Flag : 24|1@1+ (1,0) [0|0] "MODEMASK" Vector__XXX
    //  SG_ LB_Usable_SOC : 32|7@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ LB_PRUN_1DB : 48|2@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ CRC_1DB : 56|8@1+ (1,0) [0|0] "CRC" Vector__XXX

    // Byte 1 bits 8-10 LB Failsafe Status
    // 0x00 Normal start req. seems to stay on this value most of the time
    // 0x01 Normal stop req
    // 0x02 Charge stop req
    // 0x03 Charge and normal stop req. Other values call for a caution lamp which we don't need
    // bits 11-12 LB relay cut req
    // 0x00 no req
    // 0x01,0x02,0x03 main relay off req
    s16fp TMP_battI = (Param::Get(Param::idc)) * 2;
    s16fp TMP_battV = (Param::Get(Param::udc)) * 4;
    bytes[0] = TMP_battI >> 8;   // MSB current. 11 bit signed MSBit first
    bytes[1] = TMP_battI & 0xE0; // LSB current bits 7-5. Dont need to mess with bits 0-4 for now as 0 works.
    bytes[2] = TMP_battV >> 8;
    bytes[3] = ((TMP_battV & 0xC0) | (0x2b)); // 0x2b should give no cut req, main rly on permission,normal p limit.
    bytes[4] = 0x40;                          // SOC for dash in Leaf. fixed val.
    bytes[5] = 0x00;
    bytes[6] = counter_1db;

    // Extra CRC in byte 7
    nissan_crc(bytes, 0x85);

    counter_1db++;
    if (counter_1db >= 4)
        counter_1db = 0;

    can->Send(0x1DB, (uint32_t *)bytes, 8);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // CAN Message 0x50B - 10ms
    /////////////////////////////////////////////////////////////////////////////////////////////////

    //  BO_ 1291 x50B: 7 VCM
    //  SG_ DiagMuxOn_VCM : 18|1@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ HCM_WakeUpSleepCmd : 30|2@1+ (1,0) [0|0] "" Vector__XXX
    //  SG_ Batt_Heater_Mail_Send_OK : 53|1@1+ (1,0) [0|0] "" Vector__XXX

    // Statistics from 2016 capture:
    //     10 00000000000000
    //     21 000002c0000000
    //    122 000000c0000000
    //    513 000006c0000000

    // Let's just send the most common one all the time
    // FIXME: This is a very sloppy implementation. Thanks. I try:)
    bytes[0] = 0x00;
    bytes[1] = 0x00;
    bytes[2] = 0x06;
    bytes[3] = 0xc0;
    bytes[4] = 0x00;
    bytes[5] = 0x00;
    bytes[6] = 0x00;

    // possible problem here as 0x50B is DLC 7....
    can->Send(0x50B, (uint32_t *)bytes, 7);

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
    bytes[0] = 0x6E;
    bytes[1] = 0x0A;
    bytes[2] = 0x05;
    bytes[3] = 0xD5;
    bytes[4] = 0x00; // may not need pairing code crap here...and we don't:)
    bytes[5] = 0x00;
    bytes[6] = counter_1dc;
    // Extra CRC in byte 7
    nissan_crc(bytes, 0x85);

    counter_1dc++;
    if (counter_1dc >= 4)
        counter_1dc = 0;

    can->Send(0x1DC, (uint32_t *)bytes, 8);

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
    //    0x70 = 3 amps ish
    //    0x6a = 1.4A
    //    0x66 = 0.5A
    //    0x65 = 0.3A
    //    0x64 = no chg
    //    so 0x64=100. 0xA0=160. so 60 decimal steps. 1 step=100W???
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // get actual voltage and voltage setpoints
    Vbatt = Param::GetInt(Param::udc);
    VbattSP = Param::GetInt(Param::Voltspnt);
    calcBMSpwr = (Vbatt * Param::GetInt(Param::BMS_ChargeLim)); // BMS charge current limit but needs to be power for most AC charger types.

    OBCpwrSP = (MIN(Param::GetInt(Param::Pwrspnt), calcBMSpwr) / 100) + 0x64;

    if (opmode == MOD_CHARGE && Param::GetInt(Param::Chgctrl) == ChargeControl::Enable)
    {
        // clamp min and max values
        if (OBCpwrSP > 0xA0)
            OBCpwrSP = 0xA0;
        else if (OBCpwrSP < 0x64)
            OBCpwrSP = 0x64;

        // if measured vbatt is less than setpoint got to max power from web ui
        if (Vbatt < VbattSP)
            OBCpwr = OBCpwrSP;

        // decrement charger power if volt setpoint is reached
        if (Vbatt >= VbattSP)
            OBCpwr--;
    }
    else
    {
        // set power to 0 if charge control is set to off or not in charge mode
        OBCpwr = 0x64;
    }

    // Commanded chg power in byte 1 and byte 0 bits 0-1. 10 bit number.
    // byte 1=0x64 and byte 0=0x00 at 0 power.
    // 0x00 chg 0ff dcdc on.
    bytes[0] = 0x30; // msg is muxed but pdm doesn't seem to care.
    bytes[1] = OBCpwr;
    bytes[2] = 0x20; // 0x20=Normal Charge
    bytes[3] = 0xAC;
    bytes[4] = 0x00;
    bytes[5] = 0x3C;
    bytes[6] = counter_1f2;
    bytes[7] = 0x8F; // may not need checksum here?

    counter_1f2++;
    if (counter_1f2 >= 4)
    {
        counter_1f2 = 0;
    }

    can->Send(0x1F2, (uint32_t *)bytes, 8);
}
 */

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
