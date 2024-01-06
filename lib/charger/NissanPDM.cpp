#include <ACAN_T4.h>
#include <Arduino.h>

#include "module.h"
#include "can_packer.h"
#include "settings.h"
#include "CRC8.h"
#include "pack.h"

BatteryPack::BatteryPack() {}

BatteryPack::BatteryPack(int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule)
{

    // printf("Initialising BatteryPack %d\n", 1);

    numModules = _numModules;
    numCellsPerModule = _numCellsPerModule;
    numTemperatureSensorsPerModule = _numTemperatureSensorsPerModule;

    // Initialise modules
    for (int m = 0; m < numModules; m++)
    {
        // printf("Creating module %d (cpm:%d, tpm:%d)\n", m, numCellsPerModule, numTemperatureSensorsPerModule);
        modules[m] = BatteryModule(m, this);
    }
}

void BatteryPack::initialize()
{
    state = INIT;
    dtc = DTC_PACK_NONE;

    // Set up CAN port
    ACAN_T4_Settings settings(500 * 1000); // 125 kbit/s
    const uint32_t errorCode = ACAN_T4::BATTERY_CAN.begin(settings);

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
        Serial.println("Battery CAN ok");
    }
    else
    {
        Serial.print("Error Battery CAN: 0x");
        Serial.println(errorCode, HEX);
        dtc |= DTC_PACK_CAN_INIT_ERROR;
    }

    inStartup = true;
    modulePollingCycle = 0;
    moduleToPoll = 0;

    balanceActive = false;
    balanceTargetVoltage = 4.27;

    crc8.begin();
}

void BatteryPack::print()
{
    Serial.printf("Pack: %3.2fV : %dmV : %s : %s\n", voltage, cellDelta, this->getStateString(), this->getDTCString().c_str());
    for (int m = 0; m < numModules; m++)
    {
        modules[m].print();
    }
    Serial.println("");
}

// Calculate the checksum for a message to be sent out.
uint8_t BatteryPack::getcheck(CANMessage &msg, int id)
{
    unsigned char canmes[11];
    int meslen = msg.len + 1; // remove one for crc and add two for id bytes
    canmes[1] = msg.id;
    canmes[0] = msg.id >> 8;

    for (int i = 0; i < (msg.len - 1); i++)
    {
        canmes[i + 2] = msg.data[i];
    }
    return (crc8.get_crc8(canmes, meslen, finalxor[id]));
}

// Poll data from our CMU units by sending out the right CAN messages
void BatteryPack::request_data()
{
    // modulePollinCycle is the frameCounter send to the BMS
    if (modulePollingCycle == 0xF)
    {
        modulePollingCycle = 0;
    }

    // numModules counts from 0 to 7 for a BMW i3
    if (moduleToPoll == numModules)
    {
        moduleToPoll = 0;
        modulePollingCycle++;
    }

    pollModuleFrame.id = 0x080 | (moduleToPoll);
    pollModuleFrame.len = 8;

    // Byte 0 & 1: hold the target voltage for balancing
    if (balanceActive == true)
    {
        pollModuleFrame.data[0] = lowByte((uint16_t(balanceTargetVoltage * 1000) + 0));
        pollModuleFrame.data[1] = highByte((uint16_t(balanceTargetVoltage * 1000) + 0));
    }
    else
    {
        pollModuleFrame.data[0] = 0xC7;
        pollModuleFrame.data[1] = 0x10;
    }

    // Byte 2: is always 0x00
    pollModuleFrame.data[2] = 0x00;

    // Byte 3: For the first three messages 0x00. Then 0x50 to signalize that temperature and voltage should be measured.
    // Byte 4: For the first three messages 0x00. Then 0x08 if balancing active or 0x00 if balancing is inactive
    if (inStartup)
    {
        pollModuleFrame.data[3] = 0x00;
        pollModuleFrame.data[4] = 0x00;
    }
    else
    {
        pollModuleFrame.data[3] = 0x50; // 0x00 request no measurements, 0x50 request voltage and temp, 0x10 request voltage measurement, 0x40 request temperature measurement.//balancing bits
        if (balanceActive == 1)
        {
            pollModuleFrame.data[4] = 0x08; // 0x00 request no balancing
        }
        else
        {
            pollModuleFrame.data[4] = 0x00;
        }
    }

    // Byte 5: is alsways 0x00
    pollModuleFrame.data[5] = 0x00;

    // Byte 6: has the frame counter in the higher nibble. One bit is set at the end of startup.
    pollModuleFrame.data[6] = modulePollingCycle << 4;
    if (inStartup && (modulePollingCycle == 2))
    {
        pollModuleFrame.data[6] = pollModuleFrame.data[6] + 0x04;
    }

    // Byte 7: is the checksum
    pollModuleFrame.data[7] = getcheck(pollModuleFrame, moduleToPoll);

    send_message(&pollModuleFrame);

    // Only run this one, for the last module being polled
    if (inStartup && (modulePollingCycle == 2) && (moduleToPoll == (numModules - 1)))
    {
        inStartup = false;
    }

    moduleToPoll++;
    return;
}

// Read messages into modules and check alive
void BatteryPack::read_message()
{
    CANMessage msg;

    if (ACAN_T4::BATTERY_CAN.receive(msg))
    {
        for (int i = 0; i < numModules; i++)
        {
            modules[i].process_message(msg);
        }
    }

    for (int i = 0; i < numModules; i++)
    {
        modules[i].check_alive();
    }

    switch (this->state)
    {
    case INIT: // Wait for all modules to go into init
    {
        byte numModulesOperating = 0;
        for (int i = 0; i < numModules; i++)
        {
            float v = modules[i].get_voltage();
            if (modules[i].getState() == BatteryModule::OPERATING)
            {
                numModulesOperating++;
            }
            if (modules[i].getState() == BatteryModule::FAULT)
            {
                dtc |= DTC_PACK_MODULE_FAULT;
            }
        }

        if (numModulesOperating == PACK_WAIT_FOR_NUM_MODULES)
        {
            state = OPERATING;
        }
        if (dtc > 0)
        {
            state = FAULT;
        }
        break;
    }
    case OPERATING: // Check if no module fault is popping up
    {
        for (int i = 0; i < numModules; i++)
        {
            if (modules[i].getState() == BatteryModule::FAULT)
            {
                dtc |= DTC_PACK_MODULE_FAULT;
            }
        }
        if (dtc > 0)
        {
            state = FAULT;
        }
        break;
    }
    case FAULT:
    {
        // Additional fault handling logic can be added here if needed
        break;
    }
    }
}
    // Helper to send CAN message
    void BatteryPack::send_message(CANMessage * frame)
    {
        if (ACAN_T4::BATTERY_CAN.tryToSend(*frame))
        {
            // Serial.println("Send ok");
        }
        else
        {
            dtc |= DTC_PACK_CAN_SEND_ERROR;
        }
    }

    void BatteryPack::set_balancing_active(bool status)
    {
        balanceActive = status;
    }

    void BatteryPack::set_balancing_voltage(float voltage)
    {
        balanceTargetVoltage = voltage;
    }

    // Return the voltage of the lowest cell in the pack
    float BatteryPack::get_lowest_cell_voltage()
    {
        float lowestCellVoltage = 10.0000f;
        for (int m = 0; m < numModules; m++)
        {
            // skip modules with incomplete cell data
            if (modules[m].getState() != BatteryModule::OPERATING)
            {
                continue;
            }
            if (modules[m].get_lowest_cell_voltage() < lowestCellVoltage)
            {
                lowestCellVoltage = modules[m].get_lowest_cell_voltage();
            }
        }
        return lowestCellVoltage;
    }

    // Return the voltage of the highest cell in the pack
    float BatteryPack::get_highest_cell_voltage()
    {
        float highestCellVoltage = 0.0000f;
        for (int m = 0; m < numModules; m++)
        {
            // skip modules with incomplete cell data
            if (modules[m].getState() != BatteryModule::OPERATING)
            {
                continue;
            }
            if (modules[m].get_highest_cell_voltage() > highestCellVoltage)
            {
                highestCellVoltage = modules[m].get_highest_cell_voltage();
            }
        }
        return highestCellVoltage;
    }

    // return the temperature of the lowest sensor in the pack
    float BatteryPack::get_lowest_temperature()
    {
        float lowestModuleTemperature = 1000.0f;
        for (int m = 0; m < numModules; m++)
        {
            if (modules[m].getState() != BatteryModule::OPERATING)
            {
                continue;
            }
            if (modules[m].get_lowest_temperature() < lowestModuleTemperature)
            {
                lowestModuleTemperature = modules[m].get_lowest_temperature();
            }
        }
        return lowestModuleTemperature;
    }

    // return the temperature of the highest sensor in the pack
    float BatteryPack::get_highest_temperature()
    {
        float highestModuleTemperature = -50.0f;
        for (int m = 0; m < numModules; m++)
        {
            if (modules[m].getState() != BatteryModule::OPERATING)
            {
                continue;
            }
            if (modules[m].get_highest_temperature() > highestModuleTemperature)
            {
                highestModuleTemperature = modules[m].get_highest_temperature();
            }
        }
        return highestModuleTemperature;
    }

    const char *BatteryPack::getStateString()
    {
        switch (state)
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

    String BatteryPack::getDTCString()
    {
        String errorString = "";

        if (static_cast<uint8_t>(dtc) == 0)
        {
            errorString += "None";
        }
        else
        {
            if (dtc & DTC_PACK_CAN_SEND_ERROR)
            {
                errorString += "DTC_PACK_CAN_SEND_ERROR, ";
            }
            if (dtc & DTC_PACK_CAN_INIT_ERROR)
            {
                errorString += "DTC_PACK_CAN_INIT_ERROR, ";
            }
            if (dtc & DTC_PACK_MODULE_FAULT)
            {
                errorString += "DTC_PACK_MODULE_FAULT, ";
            }

            // Remove the trailing comma and space
            errorString.remove(errorString.length() - 2);
        }
        return errorString;
    }

bool BatteryPack::get_cell_voltage(byte cellIndex, float &voltage)
{
    // Check if the cell index is valid
    if (cellIndex < 0 || cellIndex >= (numModules * numCellsPerModule))
    {
        // Invalid cell index
        return false;
    }

    // Calculate the module index and cell index within the module
    int moduleIndex = cellIndex / numCellsPerModule;
    int cellWithinModule = cellIndex % numCellsPerModule;

    // Check if the module is in operating mode
    if (modules[moduleIndex].getState() == BatteryModule::OPERATING)
    {
        // Get the voltage of the specified cell within the module
        voltage = modules[moduleIndex].get_cell_voltage(cellWithinModule);
        return true;
    }
    else
    {
        // Module is not in operating mode
        voltage = 0.0;
        return false;
    }
}


float BatteryPack::get_delta_cell_voltage()
{
    // Get the highest and lowest voltages of the entire pack
    float lowestPackVoltage = get_lowest_cell_voltage();
    float highestPackVoltage = get_highest_cell_voltage();

    // Calculate and return the voltage difference
    return highestPackVoltage - lowestPackVoltage;
}


float BatteryPack::get_pack_voltage()
{
    float totalVoltage = 0.0;

    // Iterate through all modules and sum up the voltages of operating modules
    for (int i = 0; i < numModules; ++i)
    {
        // Check if the module is in operating mode
        if (modules[i].getState() == BatteryModule::OPERATING)
        {
            totalVoltage += modules[i].get_module_voltage();
        }
    }
}

bool BatteryPack::get_cell_temperature(byte cellIndex, float &temperature)
{
    // Check if the cell index is valid
    if (cellIndex < 0 || cellIndex >= (numModules * numTemperatureSensorsPerModule))
    {
        // Invalid cell index
        return false;
    }

    // Calculate the module index and temperature sensor index within the module
    int moduleIndex = cellIndex / numTemperatureSensorsPerModule;
    int sensorWithinModule = cellIndex % numTemperatureSensorsPerModule;

    // Check if the module is in operating mode
    if (modules[moduleIndex].getState() == BatteryModule::OPERATING)
    {
        // Get the average temperature of the specified sensor within the module
        temperature = modules[moduleIndex].get_average_temperature(sensorWithinModule);
        return true;
    }
    else
    {
        // Module is not in operating mode
        temperature = 0.0;
        return false;
    }
}

bool BatteryPack::get_any_module_balancing()
{
    for (int m = 0; m < numModules; m++)
    {
        // Skip modules that are not in operation
        if (modules[m].getState() != BatteryModule::OPERATING)
        {
            continue;
        }

        // If any module is balancing, return true
        if (modules[m].get_is_balancing())
        {
            return true;
        }
    }

    // If no module is balancing, return false
    return false;
}



/*
 * This file is part of the ZombieVerter project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *               2021-2022 Damien Maguire <info@evbmw.com>
 * Yes I'm really writing software now........run.....run away.......
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NissanPDM.h"
#include "my_fp.h"
#include "my_math.h"
#include "stm32_can.h"
#include "params.h"
#include "utils.h"

static uint16_t Vbatt=0;
static uint16_t VbattSP=0;
static uint8_t counter_1db=0;
static uint8_t counter_1dc=0;
static uint8_t counter_1f2=0;
static uint8_t counter_55b=0;
static uint8_t OBCpwrSP=0;
static uint8_t OBCpwr=0;
static bool OBCwake = false;
static bool PPStat = false;
static uint8_t OBCVoltStat=0;
static uint8_t PlugStat=0;
static uint16_t calcBMSpwr=0;

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

void NissanPDM::SetCanInterface(CanHardware* c)
{
   can = c;
   can->RegisterUserMessage(0x679);//Leaf obc msg
   can->RegisterUserMessage(0x390);//Leaf obc msg
}

void NissanPDM::DecodeCAN(int id, uint32_t data[2])
{
   uint8_t* bytes = (uint8_t*)data;// arrgghhh this converts the two 32bit array into bytes. See comments are useful:)

   if (id == 0x679)// THIS MSG FIRES ONCE ON CHARGE PLUG INSERT
   {
      uint8_t dummyVar = bytes[0];
      dummyVar = dummyVar;
      OBCwake = true;             //0x679 is received once when we plug in if pdm is asleep so wake wakey...
   }

   /*BO_ 912 x390: 8 OBCpd
 SG_ OBC_Status_AC_Voltage : 27|2@1+ (1,0) [0|3] "status" Vector__XXX
 SG_ OBC_Flag_QC_Relay_On_Announcemen : 38|1@1+ (1,0) [0|1] "status" Vector__XXX
 SG_ OBC_Flag_QC_IR_Sensor : 47|1@1+ (1,0) [0|1] "status" Vector__XXX
 SG_ OBC_Maximum_Charge_Power_Out : 40|9@0+ (0.1,0) [0|50] "kW" Vector__XXX
 SG_ PRUN_390 : 60|2@1+ (1,0) [0|3] "PRUN" Vector__XXX
 SG_ OBC_Charge_Status : 41|6@1+ (1,0) [0|0] "status" Vector__XXX
 SG_ CSUM_390 : 56|4@1+ (1,0) [0|16] "CSUM" Vector__XXX
 SG_ OBC_Charge_Power : 0|9@0+ (0.1,0) [0|50] "kW" Vector__XXX*/
   
   else if (id == 0x390)// THIS MSG FROM PDM
   {
      OBCVoltStat = (bytes[3] >> 3) & 0x03;
      PlugStat = bytes[5] & 0x0F;
      if(PlugStat == 0x08) PPStat = true; //plug inserted
      if(PlugStat == 0x00) PPStat = false; //plug not inserted
   }
}

bool NissanPDM::ControlCharge(bool RunCh, bool ACReq)
{
   bool dummy=RunCh;
   dummy=dummy;

   int opmode = Param::GetInt(Param::opmode);
   if(opmode != MOD_CHARGE)
   {
      if(ACReq && OBCwake)
      {
         //OBCwake = false;//reset obc wake for next time
         return true;
      }
   }

   if(PPStat && ACReq) return true;
   if(!PPStat || !ACReq)
   {
      OBCwake = false;
      return false;
   }
   return false;
}


void NissanPDM::Task10Ms()
{
   int opmode = Param::GetInt(Param::opmode);

   uint8_t bytes[8];


   /////////////////////////////////////////////////////////////////////////////////////////////////
   // CAN Message 0x1DB

   

   //We need to send 0x1db here with voltage measured by inverter
   //Zero seems to work also on my gen1
   ////////////////////////////////////////////////////////////////
   //Byte 1 bits 8-10 LB Failsafe Status
   //0x00 Normal start req. seems to stay on this value most of the time
   //0x01 Normal stop req
   //0x02 Charge stop req
   //0x03 Charge and normal stop req. Other values call for a caution lamp which we don't need
   //bits 11-12 LB relay cut req
   //0x00 no req
   //0x01,0x02,0x03 main relay off req
   s16fp TMP_battI = (Param::Get(Param::idc))*2;
   s16fp TMP_battV = (Param::Get(Param::udc))*4;
   bytes[0] = TMP_battI >> 8;     //MSB current. 11 bit signed MSBit first
   bytes[1] = TMP_battI & 0xE0;  //LSB current bits 7-5. Dont need to mess with bits 0-4 for now as 0 works.
   bytes[2] = TMP_battV >> 8;
   bytes[3] = ((TMP_battV & 0xC0) | (0x2b)); //0x2b should give no cut req, main rly on permission,normal p limit.
   bytes[4] = 0x40;  //SOC for dash in Leaf. fixed val.
   bytes[5] = 0x00;
   bytes[6] = counter_1db;

   // Extra CRC in byte 7
   nissan_crc(bytes, 0x85);


   counter_1db++;
   if(counter_1db >= 4) counter_1db = 0;

   can->Send(0x1DB, (uint32_t*)bytes, 8);

   /////////////////////////////////////////////////////////////////////////////////////////////////
   // CAN Message 0x50B

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

   //possible problem here as 0x50B is DLC 7....
   can->Send(0x50B, (uint32_t*)bytes, 7);


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
   bytes[0]=0x6E;
   bytes[1]=0x0A;
   bytes[2]=0x05;
   bytes[3]=0xD5;
   bytes[4]=0x00;//may not need pairing code crap here...and we don't:)
   bytes[5]=0x00;
   bytes[6]=counter_1dc;
   // Extra CRC in byte 7
   nissan_crc(bytes, 0x85);

   counter_1dc++;
   if (counter_1dc >= 4)
      counter_1dc = 0;

   can->Send(0x1DC, (uint32_t*)bytes, 8);

   /////////////////////////////////////////////////////////////////////////////////////////////////
   // CAN Message 0x1F2: Charge Power and DC/DC Converter Control

//    BO_ 498 x1F2: 8 VCM
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
   calcBMSpwr=(Vbatt * Param::GetInt(Param::BMS_ChargeLim));//BMS charge current limit but needs to be power for most AC charger types.


   OBCpwrSP = (MIN(Param::GetInt(Param::Pwrspnt),calcBMSpwr) / 100) + 0x64;

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
   bytes[0] = 0x30;  // msg is muxed but pdm doesn't seem to care.
   bytes[1] = OBCpwr;
   bytes[2] = 0x20;//0x20=Normal Charge
   bytes[3] = 0xAC;
   bytes[4] = 0x00;
   bytes[5] = 0x3C;
   bytes[6] = counter_1f2;
   bytes[7] = 0x8F;  //may not need checksum here?

   counter_1f2++;
   if (counter_1f2 >= 4)
   {
      counter_1f2 = 0;
   }

   can->Send(0x1F2, (uint32_t*)bytes, 8);
}

void NissanPDM::Task100Ms()
{

   // MSGS for charging with pdm
   uint8_t bytes[8];

   /////////////////////////////////////////////////////////////////////////////////////////////////
   // CAN Message 0x55B:

   bytes[0] = 0xA4;
   bytes[1] = 0x40;
   bytes[2] = 0xAA;
   bytes[3] = 0x00;
   bytes[4] = 0xDF;
   bytes[5] = 0xC0;
   bytes[6] = ((0x1 << 4) | (counter_55b));
   // Extra CRC in byte 7
   nissan_crc(bytes, 0x85);

   counter_55b++;
   if(counter_55b >= 4) counter_55b = 0;

   can->Send(0x55b, (uint32_t*)bytes, 8);

   /////////////////////////////////////////////////////////////////////////////////////////////////
   // CAN Message 0x59E:

   bytes[0] = 0x00;//Static msg works fine here
   bytes[1] = 0x00;//Batt capacity for chg and qc.
   bytes[2] = 0x0c;
   bytes[3] = 0x76;
   bytes[4] = 0x18;
   bytes[5] = 0x00;
   bytes[6] = 0x00;
   bytes[7] = 0x00;

   can->Send(0x59e, (uint32_t*)bytes, 8);

   /////////////////////////////////////////////////////////////////////////////////////////////////
   // CAN Message 0x5BC:

   // muxed msg with info for gids etc. Will try static for a test.
   bytes[0] = 0x3D;//Static msg works fine here
   bytes[1] = 0x80;
   bytes[2] = 0xF0;
   bytes[3] = 0x64;
   bytes[4] = 0xB0;
   bytes[5] = 0x01;
   bytes[6] = 0x00;
   bytes[7] = 0x32;

   can->Send(0x5bc, (uint32_t*)bytes, 8);
}

int8_t NissanPDM::fahrenheit_to_celsius(uint16_t fahrenheit)
{
   int16_t result = ((int16_t)fahrenheit - 32) * 5 / 9;
   if(result < -128)
      return -128;
   if(result > 127)
      return 127;
   return result;
}

void NissanPDM::nissan_crc(uint8_t *data, uint8_t polynomial)
{
   // We want to process 8 bytes with the 8th byte being zero
   data[7] = 0;
   uint8_t crc = 0;
   for(int b=0; b<8; b++)
   {
      for(int i=7; i>=0; i--)
      {
         uint8_t bit = ((data[b] &(1 << i)) > 0) ? 1 : 0;
         if(crc >= 0x80)
            crc = (uint8_t)(((crc << 1) + bit) ^ polynomial);
         else
            crc = (uint8_t)((crc << 1) + bit);
      }
   }
   data[7] = crc;
}


