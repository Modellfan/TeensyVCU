#ifndef CONSOLE_PRINTER_H
#define CONSOLE_PRINTER_H

#include <Arduino.h>
#include <stdarg.h>

class ConsolePrinter {
public:
    void print(const char *msg) {
        Serial.print(msg);
    }
    void println(const char *msg) {
        Serial.println(msg);
    }
    void printf(const char *fmt, ...) {
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        Serial.print(buf);
    }
};

extern ConsolePrinter console;

#endif // CONSOLE_PRINTER_H
