#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include <Arduino.h>
#include "comms_bms.h" // for external declarations
#include "console_printer.h"

void enable_serial_console();
void serial_console();
void print_console_help();
void print_pack_status();
void print_module_status(int index);

#endif // SERIAL_CONSOLE_H
