#include <ACAN_T4.h>
#include <Arduino.h>

#include "bms/battery_manager.h"
#include "bms/current.h"
#include "bms/contactor_manager.h"
#include "utils/can_packer.h"
#include <math.h>

// #define DEBUG

BMS::BMS(BatteryPack &_batteryPack, Shunt_ISA_iPace &_shunt, Contactormanager &_contactorManager) : batteryPack(_batteryPack), shunt(_shunt), contactorManager(_contactorManager)
{
    state = INIT;
    dtc = DTC_BMS_NONE;
    moduleToBeMonitored = 0;
}

void BMS::initialize()
{
    // Set up CAN port
    ACAN_T4_Settings settings(500 * 1000); // 500 kbit/s
    settings.mTransmitBufferSize = 800;
    const uint32_t errorCode = ACAN_T4::BMS_CAN.begin(settings);

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
        Serial.println("BMS CAN ok");
    }
    else
    {
        Serial.print("Error BMS CAN: 0x");
        Serial.println(errorCode, HEX);
        dtc |= DTC_BMS_CAN_INIT_ERROR;
    }

    contactorManager.close();
}

void BMS::Task2Ms() { read_message(); } // Read Can messages ?
void BMS::Task10Ms()
{
    float _current;
    // todo Check if current sensor state is operation
    _current = shunt.getCurrent();
    power = _current * batteryPack.get_pack_voltage();

    float _temp;
    _temp = shunt.getAmpereSeconds();
    ampere_seconds += _temp;
    watt_seconds += _temp * batteryPack.get_pack_voltage();

    soc_coulomb_counting = watt_seconds / total_capacity * 100; // SOC percentage

}

void BMS::Task100Ms()
{
    float _temperature;
    float _voltage;
    float _current;

    // todo Check if current sensor state is operation
    // maybe load balance this task by doing one module every 10ms

    _current = shunt.getCurrent();

    for (int i = 0; i <= CELLS_PER_MODULE * CELLS_PER_MODULE; ++i)
    {
        if (batteryPack.get_cell_voltage(i, _voltage) == false)
        {
            cell_available[i] = false;
            continue;
        }
        if (batteryPack.get_cell_temperature(i, _temperature) == false)
        {
            cell_available[i] = false;
            continue;
        }
        // if cell value is fine
        internal_resistance[i] = 1.0; // make it a better calculation. in mOhm
        open_circuite_voltage[i] = _voltage - internal_resistance[i] * _current / 1000.0;
    }
}

// Read messages into modules and check alive
void BMS::read_message()
{
    CANMessage msg;

    if (ACAN_T4::BMS_CAN.receive(msg))
    {
        //Incoming from Main
        //  Energy state - struct
        //  Close contactor manually/ open boolean
    }


}

void BMS::Monitor100Ms()
{
    shunt.monitor([this](const CANMessage &frame)
                  {
                      this->send_message(&frame); // Capture 'this' pointer explicitly
                  });

    contactorManager.monitor([this](const CANMessage &frame)
                  {
                      this->send_message(&frame); // Capture 'this' pointer explicitly
                  });

    CANMessage msg;

    msg.data64 = 0;
    msg.id = 1000; // Message 0x3e8 mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[0].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules0__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules0__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules0__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules0__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[0].get_balancing(0), 48, 1, false);                    // batteryPack__modules0__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(1), 49, 1, false);                    // batteryPack__modules0__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(2), 50, 1, false);                    // batteryPack__modules0__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(3), 51, 1, false);                    // batteryPack__modules0__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(4), 52, 1, false);                    // batteryPack__modules0__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(5), 53, 1, false);                    // batteryPack__modules0__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(6), 54, 1, false);                    // batteryPack__modules0__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(7), 55, 1, false);                    // batteryPack__modules0__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(8), 56, 1, false);                    // batteryPack__modules0__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(9), 57, 1, false);                    // batteryPack__modules0__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(10), 58, 1, false);                   // batteryPack__modules0__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[0].get_balancing(11), 59, 1, false);                   // batteryPack__modules0__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1001; // Message 0x3e9 mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[0].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules0__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1002; // Message 0x3ea mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[0].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules0__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1003; // Message 0x3eb mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[0].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules0__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules0__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[0].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules0__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1004; // Message 0x3ec mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[0].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules0__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[0].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules0__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[0].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules0__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[0].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules0__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1005; // Message 0x3ed mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[1].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules1__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules1__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules1__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules1__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[1].get_balancing(0), 48, 1, false);                    // batteryPack__modules1__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(1), 49, 1, false);                    // batteryPack__modules1__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(2), 50, 1, false);                    // batteryPack__modules1__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(3), 51, 1, false);                    // batteryPack__modules1__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(4), 52, 1, false);                    // batteryPack__modules1__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(5), 53, 1, false);                    // batteryPack__modules1__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(6), 54, 1, false);                    // batteryPack__modules1__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(7), 55, 1, false);                    // batteryPack__modules1__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(8), 56, 1, false);                    // batteryPack__modules1__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(9), 57, 1, false);                    // batteryPack__modules1__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(10), 58, 1, false);                   // batteryPack__modules1__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[1].get_balancing(11), 59, 1, false);                   // batteryPack__modules1__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1006; // Message 0x3ee mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[1].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules1__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1007; // Message 0x3ef mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[1].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules1__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1008; // Message 0x3f0 mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[1].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules1__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules1__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[1].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules1__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1009; // Message 0x3f1 mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[1].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules1__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[1].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules1__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[1].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules1__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[1].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules1__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1010; // Message 0x3f2 mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules2__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules2__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules2__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules2__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_balancing(0), 48, 1, false);                    // batteryPack__modules2__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(1), 49, 1, false);                    // batteryPack__modules2__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(2), 50, 1, false);                    // batteryPack__modules2__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(3), 51, 1, false);                    // batteryPack__modules2__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(4), 52, 1, false);                    // batteryPack__modules2__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(5), 53, 1, false);                    // batteryPack__modules2__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(6), 54, 1, false);                    // batteryPack__modules2__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(7), 55, 1, false);                    // batteryPack__modules2__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(8), 56, 1, false);                    // batteryPack__modules2__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(9), 57, 1, false);                    // batteryPack__modules2__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(10), 58, 1, false);                   // batteryPack__modules2__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[2].get_balancing(11), 59, 1, false);                   // batteryPack__modules2__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1011; // Message 0x3f3 mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules2__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1012; // Message 0x3f4 mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules2__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1013; // Message 0x3f5 mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules2__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules2__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[2].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules2__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1014; // Message 0x3f6 mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[2].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules2__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules2__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules2__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[2].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules2__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1015; // Message 0x3f7 mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[3].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules3__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules3__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules3__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules3__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[3].get_balancing(0), 48, 1, false);                    // batteryPack__modules3__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(1), 49, 1, false);                    // batteryPack__modules3__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(2), 50, 1, false);                    // batteryPack__modules3__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(3), 51, 1, false);                    // batteryPack__modules3__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(4), 52, 1, false);                    // batteryPack__modules3__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(5), 53, 1, false);                    // batteryPack__modules3__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(6), 54, 1, false);                    // batteryPack__modules3__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(7), 55, 1, false);                    // batteryPack__modules3__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(8), 56, 1, false);                    // batteryPack__modules3__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(9), 57, 1, false);                    // batteryPack__modules3__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(10), 58, 1, false);                   // batteryPack__modules3__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[3].get_balancing(11), 59, 1, false);                   // batteryPack__modules3__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1016; // Message 0x3f8 mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[3].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules3__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1017; // Message 0x3f9 mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[3].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules3__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1018; // Message 0x3fa mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[3].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules3__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules3__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[3].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules3__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1019; // Message 0x3fb mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[3].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules3__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[3].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules3__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[3].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules3__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[3].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules3__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1020; // Message 0x3fc mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[4].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules4__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules4__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules4__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules4__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[4].get_balancing(0), 48, 1, false);                    // batteryPack__modules4__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(1), 49, 1, false);                    // batteryPack__modules4__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(2), 50, 1, false);                    // batteryPack__modules4__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(3), 51, 1, false);                    // batteryPack__modules4__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(4), 52, 1, false);                    // batteryPack__modules4__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(5), 53, 1, false);                    // batteryPack__modules4__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(6), 54, 1, false);                    // batteryPack__modules4__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(7), 55, 1, false);                    // batteryPack__modules4__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(8), 56, 1, false);                    // batteryPack__modules4__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(9), 57, 1, false);                    // batteryPack__modules4__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(10), 58, 1, false);                   // batteryPack__modules4__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[4].get_balancing(11), 59, 1, false);                   // batteryPack__modules4__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1021; // Message 0x3fd mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[4].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules4__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1022; // Message 0x3fe mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[4].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules4__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1023; // Message 0x3ff mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[4].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules4__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules4__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[4].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules4__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1024; // Message 0x400 mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[4].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules4__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[4].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules4__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[4].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules4__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[4].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules4__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1025; // Message 0x401 mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[5].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules5__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules5__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules5__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules5__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[5].get_balancing(0), 48, 1, false);                    // batteryPack__modules5__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(1), 49, 1, false);                    // batteryPack__modules5__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(2), 50, 1, false);                    // batteryPack__modules5__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(3), 51, 1, false);                    // batteryPack__modules5__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(4), 52, 1, false);                    // batteryPack__modules5__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(5), 53, 1, false);                    // batteryPack__modules5__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(6), 54, 1, false);                    // batteryPack__modules5__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(7), 55, 1, false);                    // batteryPack__modules5__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(8), 56, 1, false);                    // batteryPack__modules5__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(9), 57, 1, false);                    // batteryPack__modules5__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(10), 58, 1, false);                   // batteryPack__modules5__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[5].get_balancing(11), 59, 1, false);                   // batteryPack__modules5__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1026; // Message 0x402 mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[5].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules5__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1027; // Message 0x403 mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[5].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules5__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1028; // Message 0x404 mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[5].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules5__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules5__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[5].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules5__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1029; // Message 0x405 mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[5].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules5__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[5].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules5__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[5].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules5__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[5].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules5__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1030; // Message 0x406 mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[6].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules6__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules6__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules6__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules6__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[6].get_balancing(0), 48, 1, false);                    // batteryPack__modules6__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(1), 49, 1, false);                    // batteryPack__modules6__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(2), 50, 1, false);                    // batteryPack__modules6__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(3), 51, 1, false);                    // batteryPack__modules6__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(4), 52, 1, false);                    // batteryPack__modules6__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(5), 53, 1, false);                    // batteryPack__modules6__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(6), 54, 1, false);                    // batteryPack__modules6__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(7), 55, 1, false);                    // batteryPack__modules6__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(8), 56, 1, false);                    // batteryPack__modules6__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(9), 57, 1, false);                    // batteryPack__modules6__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(10), 58, 1, false);                   // batteryPack__modules6__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[6].get_balancing(11), 59, 1, false);                   // batteryPack__modules6__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1031; // Message 0x407 mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[6].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules6__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1032; // Message 0x408 mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[6].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules6__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1033; // Message 0x409 mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[6].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules6__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules6__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[6].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules6__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1034; // Message 0x40a mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[6].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules6__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[6].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules6__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[6].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules6__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[6].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules6__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1035; // Message 0x40b mod_0_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[7].getState(), 0, 8, false, 1, 0);                     // batteryPack__modules7__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].getDTC(), 8, 8, false, 1, 0);                       // batteryPack__modules7__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_voltage(), 16, 16, false, 0.001, 0);            // batteryPack__modules7__get_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_internal_temperature(), 32, 16, false, 1, -40); // batteryPack__modules7__get_internal_temperature : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[7].get_balancing(0), 48, 1, false);                    // batteryPack__modules7__get_balancing0 : 48|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(1), 49, 1, false);                    // batteryPack__modules7__get_balancing1 : 49|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(2), 50, 1, false);                    // batteryPack__modules7__get_balancing2 : 50|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(3), 51, 1, false);                    // batteryPack__modules7__get_balancing3 : 51|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(4), 52, 1, false);                    // batteryPack__modules7__get_balancing4 : 52|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(5), 53, 1, false);                    // batteryPack__modules7__get_balancing5 : 53|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(6), 54, 1, false);                    // batteryPack__modules7__get_balancing6 : 54|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(7), 55, 1, false);                    // batteryPack__modules7__get_balancing7 : 55|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(8), 56, 1, false);                    // batteryPack__modules7__get_balancing8 : 56|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(9), 57, 1, false);                    // batteryPack__modules7__get_balancing9 : 57|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(10), 58, 1, false);                   // batteryPack__modules7__get_balancing10 : 58|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.modules[7].get_balancing(11), 59, 1, false);                   // batteryPack__modules7__get_balancing11 : 59|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1036; // Message 0x40c mod_0_voltage_1 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[7].get_cell_voltage(0), 0, 16, false, 0.001, 0);  // batteryPack__modules7__get_cell_voltage0 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(1), 16, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage1 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(2), 32, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage2 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(3), 48, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage3 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1037; // Message 0x40d mod_0_voltage_2 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[7].get_cell_voltage(4), 0, 16, false, 0.001, 0);  // batteryPack__modules7__get_cell_voltage4 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(5), 16, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage5 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(6), 32, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage6 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(7), 48, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage7 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1038; // Message 0x40e mod_0_voltage_3 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[7].get_cell_voltage(8), 0, 16, false, 0.001, 0);   // batteryPack__modules7__get_cell_voltage8 : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(9), 16, 16, false, 0.001, 0);  // batteryPack__modules7__get_cell_voltage9 : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(10), 32, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage10 : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.modules[7].get_cell_voltage(11), 48, 16, false, 0.001, 0); // batteryPack__modules7__get_cell_voltage11 : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1039; // Message 0x40f mod_0_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.modules[7].get_temperature(0), 0, 16, false, 1, -40);  // batteryPack__modules7__get_temperature0 : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[7].get_temperature(1), 16, 16, false, 1, -40); // batteryPack__modules7__get_temperature1 : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[7].get_temperature(2), 32, 16, false, 1, -40); // batteryPack__modules7__get_temperature2 : 32|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.modules[7].get_temperature(3), 48, 16, false, 1, -40); // batteryPack__modules7__get_temperature3 : 48|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1050; // Message 0x41a pack_state 8bits None
    msg.len = 8;
    pack(msg, batteryPack.getState(), 0, 8, false, 1, 0);                    // batteryPack__getState : 0|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.getDTC(), 8, 8, false, 1, 0);                      // batteryPack__getDTC : 8|8 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.get_balancing_voltage(), 16, 16, false, 0.001, 0); // get_balancing_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_balancing_active(), 32, 1, false);             // batteryPack__modules7__get_balancing_active : 32|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    pack(msg, batteryPack.get_any_module_balancing(), 33, 1, false);         // batteryPack__modules7__get_any_module_balancing : 33|1 little_endian unsigned scale: 1, offset: 0, unit: None, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1051; // Message 0x41b pack_voltage 8bits None
    msg.len = 8;
    pack(msg, batteryPack.get_lowest_cell_voltage(), 0, 16, false, 0.001, 0);   // batteryPack__get_lowest_cell_voltage : 0|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_highest_cell_voltage(), 16, 16, false, 0.001, 0); // batteryPack__get_highest_cell_voltage : 16|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_pack_voltage(), 32, 16, false, 0.01, 0);         // batteryPack__get_pack_voltage : 32|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    pack(msg, batteryPack.get_delta_cell_voltage(), 48, 16, false, 0.001, 0);   // batteryPack__get_delta_cell_voltage : 48|16 little_endian unsigned scale: 0.001, offset: 0, unit: Volt, None
    send_message(&msg);

    msg.data64 = 0;
    msg.id = 1052; // Message 0x41c pack_temperatures 8bits None
    msg.len = 8;
    pack(msg, batteryPack.get_lowest_temperature(), 0, 16, false, 1, -40);   // batteryPack__get_lowest_temperature : 0|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    pack(msg, batteryPack.get_highest_temperature(), 16, 16, false, 1, -40); // batteryPack__get_highest_temperature : 16|16 little_endian unsigned scale: 1, offset: -40, unit: Â°C, None
    send_message(&msg);
}

// void BMS::Monitor1000Ms()
// {
//     // Implementation for Monitor1000Ms method
// }

// void BMS::calculate_current_limits()
// {
//     float max_current_temperature_derating;
//     float max_current_dynamic_current_derating_battery;
//     float max_current_dynamic_current_derating_fuse;
//     float max_current_voltage_derating;
//     float max_current_cable_temperature_derating;
//     float max_current_limp_home;
//     float max_current_constant_max;

//     float max_current = max_current_constant_max;
//     max_current = fmin(max_current, max_current_temperature_derating);
//     max_current = fmin(max_current, max_current_dynamic_current_derating_battery);
//     max_current = fmin(max_current, max_current_dynamic_current_derating_fuse);
//     max_current = fmin(max_current, max_current_voltage_derating);
//     max_current = fmin(max_current, max_current_cable_temperature_derating);
//     max_current = fmin(max_current, max_current_limp_home);
//     //Aber auch mindestens Limp home strom

//     float min_current_temperature_derating;
//     float min_current_dynamic_current_derating_battery;
//     float min_current_dynamic_current_derating_fuse;
//     float min_current_voltage_derating;
//     float min_current_cable_temperature_derating;
//     float min_current_limp_home;
//     float min_current_constant_max;

//     float min_current = min_current_constant_max;
//     min_current = fmax(min_current, min_current_temperature_derating);
//     min_current = fmax(min_current, min_current_dynamic_current_derating_battery);
//     min_current = fmax(min_current, min_current_dynamic_current_derating_fuse);
//     min_current = fmax(min_current, min_current_voltage_derating);
//     min_current = fmax(min_current, min_current_cable_temperature_derating);
//     min_current = fmax(min_current, min_current_limp_home);
// };

// void BMS::calculate_balancing_target()
// {
//     // Implementation for calculate_balancing_target method
// }

// void BMS::calculate_soc()
// {
//     // Implementation for calculate_soc method
// }

// void BMS::calculate_soc_lut()
// {
//     // Implementation for calculate_soc_lut method
// }

// void BMS::calculate_soc_ekf()
// {
//     // Implementation for calculate_soc_ekf method
// }

// void BMS::calculate_soc_coulomb_counting()
// {
//     // Implementation for calculate_soc_coulomb_counting method
// }

// void BMS::calculate_open_circuit_voltage()
// {
//     // Implementation for calculate_open_circuit_voltage method
// }

// void BMS::calculate_soh()
// {
//     // Implementation for calculate_soh method
// }

void BMS::send_battery_status_message()
{
    // Implementation for send_outgoing_messages method

    //Messages to charger
    //-min current
    //Messages to inverter
    //-max current
    //-min current
    //-bms bollean failiure

    //HMI & Main VCU
        //max temp °C
        //min temp °C
        //delimiting temp °C
        //soc %
        //soh %
        //remaining capacity Wh
        //total capacity Wh
        //Battery Voltage V
        //Battery Current A
        //Min Current A
        //Max Current A
        //Error State

}

// void BMS::calculate_hmi_values()
// {
//     // Implementation for calculate_hmi_values method

//     // Temperature
//     // SOC
//     // SOE
// }

// void BMS::update_state_machine()
// {

        // for (int i = 0; i < numModules; i++)
    // {
    //     modules[i].check_alive();
    // }

    // switch (this->state)
    // {
    // case INIT: // Wait for all modules to go into init
    // {
    //     byte numModulesOperating = 0;
    //     for (int i = 0; i < numModules; i++)
    //     {
    //         float v = modules[i].get_voltage();
    //         if (modules[i].getState() == BatteryModule::OPERATING)
    //         {
    //             numModulesOperating++;
    //         }
    //         if (modules[i].getState() == BatteryModule::FAULT)
    //         {
    //             dtc |= DTC_PACK_MODULE_FAULT;
    //         }
    //     }

    //     if (numModulesOperating == PACK_WAIT_FOR_NUM_MODULES)
    //     {
    //         state = OPERATING;
    //     }
    //     if (dtc > 0)
    //     {
    //         state = FAULT;
    //     }
    //     break;
    // }
    // case OPERATING: // Check if no module fault is popping up
    // {
    //     for (int i = 0; i < numModules; i++)
    //     {
    //         if (modules[i].getState() == BatteryModule::FAULT)
    //         {
    //             dtc |= DTC_PACK_MODULE_FAULT;
    //         }
    //     }
    //     if (dtc > 0)
    //     {
    //         state = FAULT;
    //     }
    //     break;
    // }
    // case FAULT:
    // {
    //     // Additional fault handling logic can be added here if needed
    //     break;
    // }
    // }
// }

void BMS::send_message(CANMessage *frame)
{
    if (ACAN_T4::BMS_CAN.tryToSend(*frame))
    {
        // Serial.println("Send ok");
    }
    else
    {
        dtc |= DTC_BMS_CAN_SEND_ERROR;
        // Serial.println("Send nok");
    }
}
