#include "serial_console.h"
#include <ctype.h>

static const char *pack_state_to_string(BatteryPack::STATE_PACK state) {
    switch (state) {
        case BatteryPack::INIT: return "INIT";
        case BatteryPack::OPERATING: return "OPERATING";
        case BatteryPack::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static String pack_dtc_to_string(BatteryPack::DTC_PACK dtc) {
    String errorString = "";
    if (static_cast<uint8_t>(dtc) == 0) {
        errorString = "None";
    } else {
        if (dtc & BatteryPack::DTC_PACK_CAN_SEND_ERROR) {
            errorString += "CAN_SEND_ERROR, ";
        }
        if (dtc & BatteryPack::DTC_PACK_CAN_INIT_ERROR) {
            errorString += "CAN_INIT_ERROR, ";
        }
        if (dtc & BatteryPack::DTC_PACK_MODULE_FAULT) {
            errorString += "MODULE_FAULT, ";
        }
        errorString.remove(errorString.length() - 2);
    }
    return errorString;
}

static const char *bms_state_to_string(BMS::STATE_BMS state) {
    switch (state) {
        case BMS::INIT: return "INIT";
        case BMS::OPERATING: return "OPERATING";
        case BMS::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static String bms_dtc_to_string(BMS::DTC_BMS dtc) {
    String errorString = "";
    if (static_cast<uint8_t>(dtc) == 0) {
        errorString = "None";
    } else {
        if (dtc & BMS::DTC_BMS_CAN_SEND_ERROR) {
            errorString += "CAN_SEND_ERROR, ";
        }
        if (dtc & BMS::DTC_BMS_CAN_INIT_ERROR) {
            errorString += "CAN_INIT_ERROR, ";
        }
        if (dtc & BMS::DTC_BMS_PACK_FAULT) {
            errorString += "PACK_FAULT, ";
        }
        if (dtc & BMS::DTC_BMS_CONTACTOR_FAULT) {
            errorString += "CONTACTOR_FAULT, ";
        }
        if (dtc & BMS::DTC_BMS_SHUNT_FAULT) {
            errorString += "SHUNT_FAULT, ";
        }
        errorString.remove(errorString.length() - 2);
    }
    return errorString;
}

static const char *shunt_state_to_string(Shunt_ISA_iPace::STATE_ISA state) {
    switch (state) {
        case Shunt_ISA_iPace::INIT: return "INIT";
        case Shunt_ISA_iPace::OPERATING: return "OPERATING";
        case Shunt_ISA_iPace::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static String shunt_dtc_to_string(Shunt_ISA_iPace::DTC_ISA dtc) {
    String errorString = "";
    if (dtc == Shunt_ISA_iPace::DTC_ISA_NONE) {
        errorString = "None";
    } else {
        bool hasError = false;
        if (dtc & Shunt_ISA_iPace::DTC_ISA_CAN_INIT_ERROR) {
            errorString += "CAN_INIT_ERROR, ";
            hasError = true;
        }
        if (dtc & Shunt_ISA_iPace::DTC_ISA_TEMPERATURE_TOO_HIGH) {
            errorString += "TEMP_TOO_HIGH, ";
            hasError = true;
        }
        if (dtc & Shunt_ISA_iPace::DTC_ISA_MAX_CURRENT_EXCEEDED) {
            errorString += "MAX_CURRENT_EXCEEDED, ";
            hasError = true;
        }
        if (dtc & Shunt_ISA_iPace::DTC_ISA_TIMED_OUT) {
            errorString += "TIMED_OUT, ";
            hasError = true;
        }
        if (hasError) {
            errorString.remove(errorString.length() - 2);
        }
    }
    return errorString;
}

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
    console.println("  i - print current sensor status");
    console.println("  h - print this help message");
}

void print_pack_status() {
    console.printf("Pack Voltage: %3.3fV, Lowest Cell: %3.3fV, Highest Cell: %3.3fV\n",
                   batteryPack.get_pack_voltage(),
                   batteryPack.get_lowest_cell_voltage(),
                   batteryPack.get_highest_cell_voltage());
    console.printf("Lowest Temp: %.1fC, Highest Temp: %.1fC, Avg Temp: %.1fC\n",
                   batteryPack.get_lowest_temperature(),
                   batteryPack.get_highest_temperature(),
                   batteryPack.get_average_temperature());
    console.printf("Balancing Target: %3.3fV, Balancing Active: %d, Any Module Balancing: %d\n",
                   batteryPack.get_balancing_voltage(),
                   batteryPack.get_balancing_active(),
                   batteryPack.get_any_module_balancing());
    console.printf("State: %s, DTC: %s\n",
                   pack_state_to_string(batteryPack.getState()),
                   pack_dtc_to_string(batteryPack.getDTC()).c_str());
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

static const char *contactor_state_to_string(Contactor::State state) {
    switch (state) {
        case Contactor::INIT: return "INIT";
        case Contactor::OPEN: return "OPEN";
        case Contactor::CLOSING: return "CLOSING";
        case Contactor::CLOSED: return "CLOSED";
        case Contactor::OPENING: return "OPENING";
        case Contactor::FAULT: return "FAULT";
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

static String single_contactor_dtc_to_string(Contactor::DTC_CON dtc) {
    String errorString = "";
    if (dtc == Contactor::DTC_CON_NONE) {
        errorString = "None";
    } else {
        bool hasError = false;
        if (dtc & Contactor::DTC_CON_INIT_CLOSED) {
            errorString += "INIT_CLOSED, ";
            hasError = true;
        }
        if (dtc & Contactor::DTC_CON_UNEXPECTED_CLOSED) {
            errorString += "UNEXPECTED_CLOSED, ";
            hasError = true;
        }
        if (dtc & Contactor::DTC_CON_UNEXPECTED_OPEN) {
            errorString += "UNEXPECTED_OPEN, ";
            hasError = true;
        }
        if (dtc & Contactor::DTC_CON_OPEN_TIMEOUT) {
            errorString += "OPEN_TIMEOUT, ";
            hasError = true;
        }
        if (dtc & Contactor::DTC_CON_CLOSE_TIMEOUT) {
            errorString += "CLOSE_TIMEOUT, ";
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
    console.printf("BMS State: %s, DTC: %s\n",
                   bms_state_to_string(battery_manager.get_state()),
                   bms_dtc_to_string(battery_manager.get_dtc()).c_str());
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
        battery_manager.get_soc(),
        battery_manager.get_soc_ocv_lut(),
        battery_manager.get_soc_coulomb_counting());
    console.printf(
        "Current Limits - Peak Discharge: %.1fA, RMS Discharge: %.1fA, Peak Charge: %.1fA, RMS Charge: %.1fA\n",
        battery_manager.get_current_limit_peak_discharge(),
        battery_manager.get_current_limit_rms_discharge(),
        battery_manager.get_current_limit_peak_charge(),
        battery_manager.get_current_limit_rms_charge());
    console.printf("Balancing Finished: %d\n",
                   battery_manager.is_balancing_finished());

    // Also dump CAN data and internal state for debugging
    print_bms_can_data();
    print_bms_internal_state();
}

void print_bms_can_data() {
#ifdef DEBUG
    Serial.println("---- BMS CAN Data ----");
    Serial.print("pack_voltage: ");
    Serial.println(batteryPack.get_pack_voltage());
    Serial.print("pack_current: ");
    Serial.println(shunt.getCurrent());
    Serial.print("lowest_cell_voltage: ");
    Serial.println(batteryPack.get_lowest_cell_voltage());
    Serial.print("highest_cell_voltage: ");
    Serial.println(batteryPack.get_highest_cell_voltage());
    Serial.print("lowest_temp: ");
    Serial.println(batteryPack.get_lowest_temperature());
    Serial.print("highest_temp: ");
    Serial.println(batteryPack.get_highest_temperature());
    Serial.print("avg_cell_voltage: ");
    Serial.println(batteryPack.get_pack_voltage() /
                   (MODULES_PER_PACK * CELLS_PER_MODULE));
    Serial.print("cell_delta: ");
    Serial.println(batteryPack.get_delta_cell_voltage());
    Serial.print("pack_power: ");
    Serial.println(batteryPack.get_pack_voltage() * shunt.getCurrent() / 1000.0f);
    Serial.print("max_discharge_current: ");
    Serial.println(battery_manager.get_max_discharge_current());
    Serial.print("max_charge_current: ");
    Serial.println(battery_manager.get_max_charge_current());
    Serial.print("contactor_state: ");
    Serial.println(static_cast<uint8_t>(contactor_manager.getState()));
    Serial.print("dtc: ");
    Serial.println(static_cast<uint8_t>(battery_manager.get_dtc()));
    Serial.print("soc: ");
    Serial.println(battery_manager.get_soc());
    Serial.print("soh: ");
    Serial.println(100);
    Serial.print("balancing_finished: ");
    Serial.println(battery_manager.is_balancing_finished());
    Serial.print("state: ");
    Serial.println(battery_manager.get_state());
#endif
}

void print_bms_internal_state() {
#ifdef DEBUG
    // Only print diagnostics when the BMS is operating
    if (battery_manager.get_state() != BMS::OPERATING) {
        return;
    }

    Serial.println("---- BMS Internal State ----");
    Serial.print("state: ");
    Serial.println(battery_manager.get_state());
    Serial.print("dtc: ");
    Serial.println(battery_manager.get_dtc());
    Serial.print("vehicle_state: ");
    Serial.println(battery_manager.get_vehicle_state());
    Serial.print("ready_to_shutdown: ");
    Serial.println(battery_manager.get_ready_to_shutdown());
    Serial.print("vcu_timeout: ");
    Serial.println(battery_manager.get_vcu_timeout());
    Serial.print("pack_voltage: ");
    Serial.println(batteryPack.get_pack_voltage());
    Serial.print("pack_current: ");
    Serial.println(shunt.getCurrent());
    Serial.print("max_charge_current: ");
    Serial.println(battery_manager.get_max_charge_current());
    Serial.print("max_discharge_current: ");
    Serial.println(battery_manager.get_max_discharge_current());
    Serial.print("soc: ");
    Serial.println(battery_manager.get_soc());
    Serial.print("soc_ocv_lut: ");
    Serial.println(battery_manager.get_soc_ocv_lut());
    Serial.print("soc_coulomb_counting: ");
    Serial.println(battery_manager.get_soc_coulomb_counting());
    Serial.print("current_limit_peak_discharge: ");
    Serial.println(battery_manager.get_current_limit_peak_discharge());
    Serial.print("current_limit_rms_discharge: ");
    Serial.println(battery_manager.get_current_limit_rms_discharge());
    Serial.print("current_limit_peak_charge: ");
    Serial.println(battery_manager.get_current_limit_peak_charge());
    Serial.print("current_limit_rms_charge: ");
    Serial.println(battery_manager.get_current_limit_rms_charge());
    Serial.print("current_limit_rms_derated_discharge: ");
    Serial.println(battery_manager.get_current_limit_rms_derated_discharge());
    Serial.print("current_limit_rms_derated_charge: ");
    Serial.println(battery_manager.get_current_limit_rms_derated_charge());
#endif
}

void print_contactor_status() {
    console.printf("Contactor State: %s\n",
                   contactor_state_to_string(contactor_manager.getState()));
    console.printf("Contactor DTC: %s\n",
                   contactor_dtc_to_string(contactor_manager.getDTC()).c_str());
    console.printf("POS_DTC:%s PRE_DTC:%s\n",
                   single_contactor_dtc_to_string(contactor_manager.getPositiveDTC()).c_str(),
                   single_contactor_dtc_to_string(contactor_manager.getPrechargeDTC()).c_str());
    console.printf("POS:%s POS_IN:%d PRE:%s PRE_IN:%d NEG_IN:%d SUPPLY_IN:%d\n",
                   contactor_state_to_string(contactor_manager.getPositiveState()),
                   contactor_manager.getPositiveInputPin(),
                   contactor_state_to_string(contactor_manager.getPrechargeState()),
                   contactor_manager.getPrechargeInputPin(),
                   contactor_manager.isNegativeContactorClosed(),
                   contactor_manager.isContactorVoltageAvailable());
}

void print_shunt_status() {
    console.printf("Current: %.1fA (avg %.1fA)\n",
                   shunt.getCurrent(),
                   shunt.getCurrentAverage());
    console.printf("Temperature: %.1fC, AmpereSeconds: %.1fAs, dI/dt: %.1fA/s\n",
                   shunt.getTemperature(),
                   shunt.getAmpereSeconds(),
                   shunt.getCurrentDerivative());
    console.printf("State: %s, DTC: %s\n",
                   shunt_state_to_string(shunt.getState()),
                   shunt_dtc_to_string(shunt.getDTC()).c_str());
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
            case 'i':
                print_shunt_status();
                break;
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
