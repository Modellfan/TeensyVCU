#include "serial_console.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "bms/hv_monitor.h"

#define SERIAL_CONSOLE_STRINGIFY_INNER(x) #x
#define SERIAL_CONSOLE_STRINGIFY(x) SERIAL_CONSOLE_STRINGIFY_INNER(x)

#ifdef VERSION
static const char FIRMWARE_VERSION[] = SERIAL_CONSOLE_STRINGIFY(VERSION);
#else
static const char FIRMWARE_VERSION[] = "unknown";
#endif
static const char FIRMWARE_BUILD_DATE[] = __DATE__;
static const char FIRMWARE_BUILD_TIME[] = __TIME__;

#undef SERIAL_CONSOLE_STRINGIFY
#undef SERIAL_CONSOLE_STRINGIFY_INNER

void enable_serial_console() {
    console.printf("Firmware version: %s (built %s %s)\n",
                   FIRMWARE_VERSION,
                   FIRMWARE_BUILD_DATE,
                   FIRMWARE_BUILD_TIME);
    console.println("Type 'h' for help.");
}

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

static const char *vehicle_state_to_string(BMS::VehicleState state) {
    switch (state) {
        case BMS::STATE_SLEEP: return "SLEEP";
        case BMS::STATE_STANDBY: return "STANDBY";
        case BMS::STATE_READY: return "READY";
        case BMS::STATE_CONDITIONING: return "CONDITIONING";
        case BMS::STATE_DRIVE: return "DRIVE";
        case BMS::STATE_CHARGE: return "CHARGE";
        case BMS::STATE_ERROR: return "ERROR";
        case BMS::STATE_LIMP_HOME: return "LIMP_HOME";
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

static const char *shunt_state_to_string(ShuntState state) {
    switch (state) {
        case ShuntState::INIT: return "INIT";
        case ShuntState::OPERATING: return "OPERATING";
        case ShuntState::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static String shunt_dtc_to_string(ShuntDTC dtc) {
    String errorString = "";
    if (dtc == SHUNT_DTC_NONE) {
        errorString = "None";
    } else {
        bool hasError = false;
        if (dtc & SHUNT_DTC_CAN_INIT_ERROR) {
            errorString += "CAN_INIT_ERROR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_TEMPERATURE_TOO_HIGH) {
            errorString += "TEMP_TOO_HIGH, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_MAX_CURRENT_EXCEEDED) {
            errorString += "MAX_CURRENT_EXCEEDED, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_TIMED_OUT) {
            errorString += "TIMED_OUT, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_I_ERROR) {
            errorString += "I_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_U1_ERROR) {
            errorString += "U1_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_U2_ERROR) {
            errorString += "U2_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_U3_ERROR) {
            errorString += "U3_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_T_ERROR) {
            errorString += "T_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_W_ERROR) {
            errorString += "W_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_AS_ERROR) {
            errorString += "AS_STATUS_ERR, ";
            hasError = true;
        }
        if (dtc & SHUNT_DTC_STATUS_WH_ERROR) {
            errorString += "WH_STATUS_ERR, ";
            hasError = true;
        }
        if (hasError) {
            errorString.remove(errorString.length() - 2);
        }
    }
    return errorString;
}

static const char *hv_monitor_state_to_string(HVMonitorState state) {
    switch (state) {
        case HVMonitorState::INIT: return "INIT";
        case HVMonitorState::OPERATING: return "OPERATING";
        case HVMonitorState::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static void print_shunt_status_bits(const char *label, Shunt_IVTS::StatusBits bits) {
    console.printf("  %s: ctr=%u ocs=%u this_me_err=%u any_me_err=%u sys_err=%u\n",
                   label,
                   static_cast<unsigned int>(bits.counter),
                   bits.ocs ? 1 : 0,
                   bits.this_out_of_range_or_me_err ? 1 : 0,
                   bits.any_me_err ? 1 : 0,
                   bits.system_err ? 1 : 0);
}

static bool read_serial_token(char *buffer, size_t buffer_size) {
    size_t i = 0;
    while (Serial.available() && isspace(static_cast<unsigned char>(Serial.peek()))) {
        Serial.read();
    }

    while (Serial.available() && i < buffer_size - 1) {
        char c = Serial.peek();
        if (c == '\n' || c == '\r' || isspace(static_cast<unsigned char>(c))) {
            break;
        }
        buffer[i++] = Serial.read();
    }

    buffer[i] = '\0';
    return i > 0;
}

static bool parse_bool_token(const char *token, bool &value) {
    if (token == nullptr || token[0] == '\0') {
        return false;
    }

    char lowered[16];
    size_t len = strlen(token);
    if (len >= sizeof(lowered)) {
        len = sizeof(lowered) - 1;
    }

    bool has_digit = false;
    bool numeric = true;

    for (size_t i = 0; i < len; ++i) {
        const char c = token[i];
        lowered[i] = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (!(isdigit(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.')) {
            numeric = false;
        }
        if (isdigit(static_cast<unsigned char>(c))) {
            has_digit = true;
        }
    }
    lowered[len] = '\0';

    if (strcmp(lowered, "1") == 0 || strcmp(lowered, "true") == 0 || strcmp(lowered, "on") == 0 || strcmp(lowered, "yes") == 0) {
        value = true;
        return true;
    }
    if (strcmp(lowered, "0") == 0 || strcmp(lowered, "false") == 0 || strcmp(lowered, "off") == 0 || strcmp(lowered, "no") == 0) {
        value = false;
        return true;
    }

    if (numeric && has_digit) {
        value = (atof(token) != 0.0f);
        return true;
    }

    return false;
}

static void discard_serial_line() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            break;
        }
    }
}

void print_console_help() {
    console.printf("Firmware version: %s (built %s %s)\n",
                   FIRMWARE_VERSION,
                   FIRMWARE_BUILD_DATE,
                   FIRMWARE_BUILD_TIME);
    console.println("Available commands:");
    console.println("  c - close contactors");
    console.println("  cs - configure shunt");
    console.println("  o - open contactors");
    console.println("  s - show contactor status");
    console.println("  H - print external HV bus voltage");
    console.println("  p - print pack status");
    console.println("  b - toggle balancing");
    console.println("  vX.XX - set balancing voltage");
    console.println("  mX - print module X status (0-7)");
    console.println("  B - print BMS status");
    console.println("  i - print current sensor status");
    console.println("  P - print persistent data");
    console.println("  E idx value - set persistent data value (see 'P')");
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

static const char *contactor_manager_state_to_string(Contactormanager::State state) {
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

static const char *contactor_precharge_strategy_to_string(
    Contactormanager::PrechargeStrategy strategy) {
    switch (strategy) {
        case Contactormanager::PrechargeStrategy::TIMED_DELAY:
            return "TIMED_DELAY";
        case Contactormanager::PrechargeStrategy::VOLTAGE_MATCH:
            return "VOLTAGE_MATCH";
        default:
            return "UNKNOWN";
    }
}

static const char *single_contactor_state_to_string(Contactor::State state) {
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

static const char *contactor_feedback_to_string(bool is_closed) {
    return is_closed ? "FEEDBACK CLOSED" : "FEEDBACK OPEN";
}

static String contactor_manager_dtc_to_string(Contactormanager::DTC_COM dtc) {
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
        if (dtc & Contactormanager::DTC_COM_PRECHARGE_VOLTAGE_TIMEOUT) {
            errorString += "MATCH_TIMEOUT, ";
            hasError = true;
        }
        if (dtc & Contactormanager::DTC_COM_EXTERNAL_HV_VOLTAGE_MISSING) {
            errorString += "EXT_VOLT_MISSING, ";
            hasError = true;
        }
        if (dtc & Contactormanager::DTC_COM_PACK_VOLTAGE_MISSING) {
            errorString += "PACK_VOLT_MISSING, ";
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
        bool hasError = false;
        if (dtc & BatteryModule::DTC_CMU_INTERNAL_ERROR) {
            errorString += "INTERNAL_ERROR, ";
            hasError = true;
        }
        if (dtc & BatteryModule::DTC_CMU_TEMPERATURE_TOO_HIGH) {
            errorString += "TEMP_TOO_HIGH, ";
            hasError = true;
        }
        if (dtc & BatteryModule::DTC_CMU_SINGLE_VOLTAGE_IMPLAUSIBLE) {
            errorString += "VOLT_IMPLAUSIBLE, ";
            hasError = true;
        }
        if (dtc & BatteryModule::DTC_CMU_TEMPERATURE_IMPLAUSIBLE) {
            errorString += "TEMP_IMPLAUSIBLE, ";
            hasError = true;
        }
        if (dtc & BatteryModule::DTC_CMU_TIMED_OUT) {
            errorString += "TIMED_OUT, ";
            hasError = true;
        }
        if (dtc & BatteryModule::DTC_CMU_MODULE_VOLTAGE_IMPLAUSIBLE) {
            errorString += "MODULE_VOLT_IMPLAUSIBLE, ";
            hasError = true;
        }
        if (dtc & BatteryModule::DTC_CMU_CRC_ERROR) {
            errorString += "CRC_ERROR, ";
            hasError = true;
        }
        if (hasError) {
            errorString.remove(errorString.length() - 2);
        }
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
    console.printf("HV Monitor: %s, Voltage Matched: %d\n",
                   hv_monitor_state_to_string(param::hv_monitor_state),
                   param::voltage_matched ? 1 : 0);
    if (battery_manager.is_vcu_data_valid()) {
        console.printf(
            "Vehicle State: %s, ReadyToShutdown: %d, VCU Timeout: %d\n",
            vehicle_state_to_string(battery_manager.get_vehicle_state()),
            battery_manager.get_ready_to_shutdown(),
            battery_manager.get_vcu_timeout());
    } else {
        console.printf("Vehicle State: INVALID, VCU Timeout: %d\n",
                       battery_manager.get_vcu_timeout());
    }
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
    console.printf(
        "Derated RMS Limits - Discharge: %.1fA, Charge: %.1fA\n",
        battery_manager.get_current_limit_rms_derated_discharge(),
        battery_manager.get_current_limit_rms_derated_charge());
    console.printf("Balancing Finished: %d\n",
                   battery_manager.is_balancing_finished());
}


void print_contactor_status() {
    const Contactormanager::State manager_state = contactor_manager.getState();
    const Contactormanager::DTC_COM manager_dtc = contactor_manager.getDTC();
    const Contactor::State positive_state = contactor_manager.getPositiveState();
    const Contactor::State precharge_state = contactor_manager.getPrechargeState();
    const Contactor::DTC_CON positive_dtc = contactor_manager.getPositiveDTC();
    const Contactor::DTC_CON precharge_dtc = contactor_manager.getPrechargeDTC();
    const bool positive_feedback_closed = contactor_manager.getPositiveInputPin();
    const bool precharge_feedback_closed = contactor_manager.getPrechargeInputPin();
    const bool negative_contactor_closed = contactor_manager.isNegativeContactorClosed();
    const bool supply_available = contactor_manager.isContactorVoltageAvailable();
    const bool feedback_disabled = contactor_manager.isFeedbackDisabled();
    const Contactormanager::PrechargeStrategy precharge_strategy =
        contactor_manager.getPrechargeStrategy();
    const float voltage_match_tolerance =
        contactor_manager.getVoltageMatchTolerance();
    const uint32_t voltage_match_timeout =
        contactor_manager.getVoltageMatchTimeout();
    const float hv_bus_voltage = contactor_manager.getHvBusVoltage();
    const bool hv_bus_voltage_valid = contactor_manager.isHvBusVoltageValid();
    const float pack_voltage = contactor_manager.getPackVoltage();
    const bool pack_voltage_valid = contactor_manager.isPackVoltageValid();

    console.println("Contactor Manager:");
    console.printf("  Manager state: %s\n",
                   contactor_manager_state_to_string(manager_state));
    console.printf("  Manager DTC: %s (0x%02X)\n",
                   contactor_manager_dtc_to_string(manager_dtc).c_str(),
                   static_cast<unsigned int>(manager_dtc));
    console.printf("  Feedback disabled: %s\n", feedback_disabled ? "true" : "false");
    console.printf("  Precharge strategy: %s, Voltage match tolerance: %.3fV, Timeout: %lums\n",
                   contactor_precharge_strategy_to_string(precharge_strategy),
                   voltage_match_tolerance,
                   static_cast<unsigned long>(voltage_match_timeout));
    console.printf("  Negative contactor closed: %s, Supply available: %s\n",
                   negative_contactor_closed ? "true" : "false",
                   supply_available ? "true" : "false");
    console.printf("  HV bus voltage: %.1fV (%s)\n",
                   hv_bus_voltage,
                   hv_bus_voltage_valid ? "valid" : "invalid");
    console.printf("  Pack voltage: %.1fV (%s)\n",
                   pack_voltage,
                   pack_voltage_valid ? "valid" : "invalid");
    console.printf("  Positive contactor - State: %s, Feedback: %s (%d), DTC: %s (0x%02X)\n",
                   single_contactor_state_to_string(positive_state),
                   contactor_feedback_to_string(positive_feedback_closed),
                   positive_feedback_closed,
                   single_contactor_dtc_to_string(positive_dtc).c_str(),
                   static_cast<unsigned int>(positive_dtc));
    console.printf("  Precharge contactor - State: %s, Feedback: %s (%d), DTC: %s (0x%02X)\n",
                   single_contactor_state_to_string(precharge_state),
                   contactor_feedback_to_string(precharge_feedback_closed),
                   precharge_feedback_closed,
                   single_contactor_dtc_to_string(precharge_dtc).c_str(),
                   static_cast<unsigned int>(precharge_dtc));
}

void print_external_voltage() {
    const float hv_bus_voltage = contactor_manager.getHvBusVoltage();
    if (contactor_manager.isHvBusVoltageValid()) {
        console.printf("External HV bus voltage: %.1fV (valid)\n", hv_bus_voltage);
    } else {
        console.printf("External HV bus voltage unavailable (last %.1fV)\n", hv_bus_voltage);
    }
}

void print_shunt_status() {
    console.println("Shunt:");
    console.printf("  State: %s\n", shunt_state_to_string(param::state));
    console.printf("  DTC: %s (0x%04X)\n",
                   shunt_dtc_to_string(param::dtc).c_str(),
                   static_cast<unsigned int>(param::dtc));
    console.printf("  Current: %.3fA (avg %.3fA, dI/dt %.3fA/s)\n",
                   param::current,
                   param::current_avg,
                   param::current_dA_per_s);
    console.printf("  Voltages: U_in_hvbox %.3fV, U_out_hvbox %.3fV, U3 %.3fV\n",
                   param::u_input_hvbox,
                   param::u_output_hvbox,
                   param::u3);
    console.printf("  Temp: %.1fC, Power: %.1fW\n",
                   param::temp,
                   param::power);
    console.printf("  Charge: %.1fAs, Energy: %.1fWh\n",
                   param::as,
                   param::wh);
    console.println("  Status:");
    print_shunt_status_bits("I ", shunt.status_I());
    print_shunt_status_bits("U1", shunt.status_U1());
    print_shunt_status_bits("U2", shunt.status_U2());
    print_shunt_status_bits("U3", shunt.status_U3());
    print_shunt_status_bits("T ", shunt.status_T());
    print_shunt_status_bits("W ", shunt.status_W());
    print_shunt_status_bits("As", shunt.status_As());
    print_shunt_status_bits("Wh", shunt.status_Wh());
}

void print_persistent_data() {
    const PersistentDataStorage::PersistentData data = battery_manager.get_persistent_data();
    console.println("Persistent data:");
    console.printf("  0: energy_initial_Wh = %.3f\n", data.energy_initial_Wh);
    console.printf("  1: measured_capacity_Wh = %.3f\n", data.measured_capacity_Wh);
    console.printf("  2: ampere_seconds_initial = %.3f\n", data.ampere_seconds_initial);
    console.printf("  3: measured_capacity_Ah = %.3f\n", data.measured_capacity_Ah);
    console.printf("  4: ignore_contactor_feedback = %s\n", data.ignore_contactor_feedback ? "true" : "false");
    console.printf("  5: contactor_precharge_strategy = %u\n", data.contactor_precharge_strategy);
    console.printf("  6: contactor_voltage_match_tolerance_v = %.3f\n", data.contactor_voltage_match_tolerance_v);
    console.printf("  7: contactor_voltage_match_timeout_ms = %lu\n",
                   static_cast<unsigned long>(data.contactor_voltage_match_timeout_ms));
    console.println("Use 'E idx value' to update a field (use true/false or 1/0 for boolean values).");
}

void modify_persistent_data() {
    char index_token[8];
    if (!read_serial_token(index_token, sizeof(index_token))) {
        console.println("Persistent value index required (see 'P').");
        discard_serial_line();
        return;
    }

    char value_token[32];
    if (!read_serial_token(value_token, sizeof(value_token))) {
        console.println("New value required.");
        discard_serial_line();
        return;
    }

    discard_serial_line();

    const int index = atoi(index_token);
    const float value = atof(value_token);

    PersistentDataStorage::PersistentData data = battery_manager.get_persistent_data();
    bool updated = true;

    switch (index) {
        case 0:
            data.energy_initial_Wh = value;
            break;
        case 1:
            data.measured_capacity_Wh = value;
            break;
        case 2:
            data.ampere_seconds_initial = value;
            break;
        case 3:
            data.measured_capacity_Ah = value;
            break;
        case 4: {
            bool bool_value = false;
            if (!parse_bool_token(value_token, bool_value)) {
                console.println("Value must be boolean (true/false or 1/0).");
                return;
            }
            data.ignore_contactor_feedback = bool_value;
            break;
        }
        case 5: {
            const int strategy = atoi(value_token);
            if (strategy != CONTACTOR_PRECHARGE_STRATEGY_TIMED_DELAY &&
                strategy != CONTACTOR_PRECHARGE_STRATEGY_VOLTAGE_MATCH) {
                console.println("Value must be 0 (timed delay) or 1 (voltage match).");
                return;
            }
            data.contactor_precharge_strategy = static_cast<uint8_t>(strategy);
            break;
        }
        case 6:
            data.contactor_voltage_match_tolerance_v = value;
            break;
        case 7: {
            if (value <= 0.0f) {
                console.println("Timeout must be positive.");
                return;
            }
            data.contactor_voltage_match_timeout_ms = static_cast<uint32_t>(value);
            break;
        }
        default:
            updated = false;
            break;
    }

    if (!updated) {
        console.println("Invalid persistent value index.");
        return;
    }

    battery_manager.update_persistent_data(data);
    console.println("Persistent data updated.");
    print_persistent_data();
}


void serial_console() {
    while (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'c':
                if (Serial.available() && Serial.peek() == 's') {
                    Serial.read();
                    console.println("Configuring shunt...");
                    const bool ok = shunt.configure_shunt();
                    console.println(ok ? "Shunt configured." : "Shunt configuration failed.");
                } else {
                    console.println("Closing contactors...");
                    contactor_manager.close();
                }
                break;
            case 'o':
                console.println("Opening contactors...");
                contactor_manager.open();
                break;
            case 's':
                print_contactor_status();
                break;
            case 'H':
                print_external_voltage();
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
            case 'P':
                print_persistent_data();
                break;
            case 'E':
                modify_persistent_data();
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
