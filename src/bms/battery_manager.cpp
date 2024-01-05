#include "pack.h"  // Include the BatteryPack class or adjust as needed

class BMS
{
private:
    BatteryPack &batteryPack;  // Reference to the BatteryPack

    // Private functions
    void calculate_current_limits();
    void calculate_soc();
    void calculate_soc_lut();
    void calculate_soc_ekf();
    void calculate_soc_coulomb_counting();
    void calculate_open_circuit_voltage();
    void update_state_machine();

public:
    BMS(BatteryPack &_batteryPack);  // Constructor taking a reference to BatteryPack

    // Public functions
    void runnable_update();
    void runnable_send_CAN_message();
};


