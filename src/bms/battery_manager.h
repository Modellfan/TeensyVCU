
#ifndef BMS_H
#define BMS_H

#include <ACAN_T4.h>
#include <Arduino.h>

#include "pack.h"
#include "utils/can_packer.h"
#include "settings.h"

class BMS
{

public:
        enum STATE_BMS
        {
                INIT,      // pack is being initialized
                OPERATING, // pack is in operation state
                FAULT      // pack is in fault state
        };

        typedef enum
        {
                DTC_BMS_NONE = 0,
                DTC_BMS_CAN_SEND_ERROR = 1 << 0,
                DTC_BMS_CAN_INIT_ERROR = 1 << 1,
                DTC_BMS_PACK_FAULT = 1 << 2,
        } DTC_BMS;

        BMS(BatteryPack &_batteryPack); // Constructor taking a reference to BatteryPack

        // Runnables
        void print();
        void initialize();

        void Task2Ms();  // Read Can messages ?
        void Task10Ms(); // Poll messages
        void Task100Ms();

        void Monitor100Ms();
        void Monitor1000Ms();

private:
        BatteryPack &batteryPack; // Reference to the BatteryPack

        // Non-Volatile Variable!!
        float watt_seconds;

        // Current derating
        void calculate_current_limits();

        // Calculate balacing target
        void calculate_balancing_target();

        // Calculate SOC
        void calculate_soc();
        void calculate_soc_lut();
        void calculate_soc_ekf();
        void calculate_soc_coulomb_counting();
        void calculate_open_circuit_voltage();
        void calculate_soh();

        // Send CAN messages
        void send_outgoing_messages();
        void calculate_hmi_values();

        // State maschine updating
        void update_state_machine();

        // Helper functions
        void send_message(CANMessage *frame); // Send out CAN message

        // State and DTC
        STATE_BMS state;
        DTC_BMS dtc;
        const char *getStateString();
        String getDTCString();
};

#endif
