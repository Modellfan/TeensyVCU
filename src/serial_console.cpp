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

void print_module_status(int index) {
    if (index < 0 || index >= MODULES_PER_PACK) {
        console.println("Invalid module index");
        return;
    }
    batteryPack.modules[index].print();
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
                contactor_manager.print();
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
