#include "serial_console.h"
#include <ctype.h>

void print_console_help() {
    console.println("Available commands:");
    console.println("  c - close contactors");
    console.println("  o - open contactors");
    console.println("  s - show contactor status");
    console.println("  p - print pack status");
    console.println("  b - toggle balancing");
    console.println("  vX.XX - set balancing voltage");
    console.println("  mX - print module X status (0-7)");
    console.println("  B - print BMS status");
    console.println("  h - print this help message");
}

void print_pack_status() {
    console.printf("Pack Voltage: %3.2fV, Lowest Cell: %3.2fV, Highest Cell: %3.2fV\n",
                   batteryPack.get_pack_voltage(),
                   batteryPack.get_lowest_cell_voltage(),
                   batteryPack.get_highest_cell_voltage());
    console.printf("Balancing Target: %3.2fV, Balancing Active: %d, Any Module Balancing: %d\n",
                   batteryPack.get_balancing_voltage(),
                   batteryPack.get_balancing_active(),
                   batteryPack.get_any_module_balancing());
    console.printf("State: %d, DTC: %d\n",
                   batteryPack.getState(),
                   batteryPack.getDTC());
}

static const char *contactor_state_to_string(Contactormanager::State state) {
    switch (state) {
        case Contactormanager::INIT: return "INIT";
        case Contactormanager::OPEN: return "OPEN";
        case Contactormanager::CLOSING_PRECHARGE: return "CLOS_PRE";
        case Contactormanager::CLOSING_POSITIVE: return "CLOS_POS";
        case Contactormanager::CLOSED: return "CLOSED";
        case Contactormanager::OPENING_POSITIVE: return "OPEN_POS";
        case Contactormanager::OPENING_PRECHARGE: return "OPEN_PRE";
        case Contactormanager::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static String contactor_dtc_to_string(Contactormanager::DTC_COM dtc) {
    String errorString = "";
    if (dtc == Contactormanager::DTC_COM_NONE) {
        errorString = "None";
    } else {
        bool hasError = false;
        if (dtc & Contactormanager::DTC_COM_NO_CONTACTOR_POWER_SUPPLY) {
            errorString += "NO_POWER, ";
            hasError = true;
        }
        if (dtc & Contactormanager::DTC_COM_NEGATIVE_CONTACTOR_FAULT) {
            errorString += "NEG_FAULT, ";
            hasError = true;
        }
        if (dtc & Contactormanager::DTC_COM_PRECHARGE_CONTACTOR_FAULT) {
            errorString += "PRECHARGE_FAULT, ";
            hasError = true;
        }
        if (dtc & Contactormanager::DTC_COM_POSITIVE_CONTACTOR_FAULT) {
            errorString += "POSITIVE_FAULT, ";
            hasError = true;
        }
        if (hasError) {
            errorString.remove(errorString.length() - 2);
        }
    }
    return errorString;
}

static const char *module_state_to_string(BatteryModule::STATE_CMU state) {
    switch (state) {
        case BatteryModule::INIT: return "INIT";
        case BatteryModule::OPERATING: return "OPERATING";
        case BatteryModule::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static String module_dtc_to_string(BatteryModule::DTC_CMU dtc) {
    String errorString = "";
    if (static_cast<uint8_t>(dtc) == 0) {
        errorString = "None";
    } else {
        if (dtc & BatteryModule::DTC_CMU_INTERNAL_ERROR) {
            errorString += "INTERNAL_ERROR, ";
        }
        if (dtc & BatteryModule::DTC_CMU_TEMPERATURE_TOO_HIGH) {
            errorString += "TEMP_TOO_HIGH, ";
        }
        if (dtc & BatteryModule::DTC_CMU_SINGLE_VOLTAGE_IMPLAUSIBLE) {
            errorString += "VOLT_IMPLAUSIBLE, ";
        }
        if (dtc & BatteryModule::DTC_CMU_TEMPERATURE_IMPLAUSIBLE) {
            errorString += "TEMP_IMPLAUSIBLE, ";
        }
        if (dtc & BatteryModule::DTC_CMU_TIMED_OUT) {
            errorString += "TIMED_OUT, ";
        }
        if (dtc & BatteryModule::DTC_CMU_MODULE_VOLTAGE_IMPLAUSIBLE) {
            errorString += "MODULE_VOLT_IMPLAUSIBLE, ";
        }
        errorString.remove(errorString.length() - 2);
    }
    return errorString;
}

void print_module_status(int index) {
    if (index < 0 || index >= MODULES_PER_PACK) {
        console.println("Invalid module index");
        return;
    }
    BatteryModule &mod = batteryPack.modules[index];
    console.printf("Module %d\n", index);
    console.printf("  State: %s\n", module_state_to_string(mod.getState()));
    console.printf("  DTC: %s\n", module_dtc_to_string(mod.getDTC()).c_str());
    console.printf("  Voltage: %3.3fV\n", mod.get_voltage());
    console.printf("  Lowest Cell: %3.3fV, Highest Cell: %3.3fV\n",
                   mod.get_lowest_cell_voltage(), mod.get_highest_cell_voltage());
    console.printf("  Lowest Temp: %.1fC, Highest Temp: %.1fC, Avg Temp: %.1fC\n",
                   mod.get_lowest_temperature(), mod.get_highest_temperature(),
                   mod.get_average_temperature());
    console.printf("  Balancing: %d\n", mod.get_is_balancing());
}

void print_bms_status() {
    console.printf("BMS State: %d, DTC: %d\n",
                   battery_manager.get_state(), battery_manager.get_dtc());
    console.printf(
        "Vehicle State: %d, ReadyToShutdown: %d, VCU Timeout: %d\n",
        battery_manager.get_vehicle_state(),
        battery_manager.get_ready_to_shutdown(),
        battery_manager.get_vcu_timeout());
    console.printf("Max Charge Current: %.1fA, Max Discharge Current: %.1fA\n",
                   battery_manager.get_max_charge_current(),
                   battery_manager.get_max_discharge_current());
    console.printf(
        "SOC: %.1f%% (OCV %.1f%%, Coulomb %.1f%%)\n",
        battery_manager.get_soc() * 100.0f,
        battery_manager.get_soc_ocv_lut() * 100.0f,
        battery_manager.get_soc_coulomb_counting() * 100.0f);
    console.printf(
        "Current Limits - Peak Discharge: %.1fA, RMS Discharge: %.1fA, Peak Charge: %.1fA, RMS Charge: %.1fA\n",
        battery_manager.get_current_limit_peak_discharge(),
        battery_manager.get_current_limit_rms_discharge(),
        battery_manager.get_current_limit_peak_charge(),
        battery_manager.get_current_limit_rms_charge());
    console.printf("Balancing Finished: %d\n",
                   battery_manager.is_balancing_finished());
}

void print_contactor_status() {
    console.printf("Contactor State: %s\n",
                   contactor_state_to_string(contactor_manager.getState()));
    console.printf("Contactor DTC: %s\n",
                   contactor_dtc_to_string(contactor_manager.getDTC()).c_str());
}


void serial_console() {
    while (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'c':
                console.println("Closing contactors...");
                contactor_manager.close();
                break;
            case 'o':
                console.println("Opening contactors...");
                contactor_manager.open();
                break;
            case 's':
                print_contactor_status();
                break;
            case 'p':
                print_pack_status();
                break;
            case 'b': {
                bool newState = !batteryPack.get_balancing_active();
                batteryPack.set_balancing_active(newState);
                console.printf("Balancing %s\n", newState ? "enabled" : "disabled");
                break;
            }
            case 'v': {
                // read float value from the serial buffer
                char buf[16];
                int i = 0;
                // skip leading spaces
                while (Serial.available() && isspace(Serial.peek())) {
                    Serial.read();
                }
                while (Serial.available() && i < 15) {
                    char c = Serial.peek();
                    if (c == '\n' || c == '\r' || c == ' ') {
                        break;
                    }
                    buf[i++] = Serial.read();
                }
                buf[i] = '\0';
                float v = atof(buf);
                if (v > 0.0f) {
                    batteryPack.set_balancing_voltage(v);
                    console.printf("Balancing voltage set to %3.2fV\n", batteryPack.get_balancing_voltage());
                } else {
                    console.println("Invalid voltage");
                }
                break;
            }
            case 'm': {
                if (Serial.available()) {
                    char idxChar = Serial.read();
                    int idx = idxChar - '0';
                    print_module_status(idx);
                } else {
                    console.println("Module index required");
                }
                break;
            }
            case 'B':
                print_bms_status();
                break;
            case 'h':
            case '?':
                print_console_help();
                break;
            case '\n':
            case '\r':
                break;
            default:
                console.print("Unknown command: ");
                console.println(String(cmd).c_str());
                print_console_help();
                break;
        }
    }
}
