#ifndef GVRET_H 
#define GVRET_H

#include <Arduino.h>
#include <ACAN_T4.h>  // For CANMessage definition

// GVRET Protocol definitions
#define CFG_BUILD_NUM 7010

enum GVRET_STATE {
    IDLE,
    GET_COMMAND,
    BUILD_CAN_FRAME, // (Not fully implemented here)
    TIME_SYNC,
    GET_DIG_INPUTS,
    GET_ANALOG_INPUTS,
    SET_DIG_OUTPUTS,
    SETUP_CANBUS,
    GET_CANBUS_PARAMS,
    GET_DEVICE_INFO,
    SET_SINGLEWIRE_MODE,
    SET_SYSTYPE,
    ECHO_CAN_FRAME,
    SETUP_EXT_BUSES,
    BUILD_FD_FRAME // (Ignored in this code)
};

enum GVRET_PROTOCOL {
    PROTO_BUILD_CAN_FRAME  = 0,
    PROTO_TIME_SYNC        = 1,
    PROTO_DIG_INPUTS       = 2,
    PROTO_ANA_INPUTS       = 3,
    PROTO_SET_DIG_OUT      = 4,
    PROTO_SETUP_CANBUS     = 5,
    PROTO_GET_CANBUS_PARAMS = 6,
    PROTO_GET_DEV_INFO     = 7,
    PROTO_SET_SW_MODE      = 8,
    PROTO_KEEPALIVE        = 9,
    PROTO_SET_SYSTYPE       = 10,
    PROTO_ECHO_CAN_FRAME   = 11,
    PROTO_GET_NUMBUSES     = 12,
    PROTO_GET_EXT_BUSES    = 13,
    PROTO_SET_EXT_BUSES    = 14,
    PROTO_BUILD_FD_FRAME   = 20, // Ignored
    PROTO_SETUP_FD         = 21, // Ignored
    PROTO_GET_FD           = 22  // Ignored
};

// GVRET state variables
// (Since this header is only included in one file, we define these as static.)
static bool gvret_binOutput = false;
static GVRET_STATE gvretState = IDLE;
static int gvretStep = 0;

// gvret_loop: Process incoming USB data using a GVRET state machine.
// Note: This function uses GVRET_PORT which must be defined (see main.cpp).
inline void gvret_loop() {
    uint8_t buff[80];
    int c;
    uint32_t now;
    static int out_bus = 0;
    
    while (GVRET_PORT.available()) {
        c = GVRET_PORT.read();
        // For debugging you can uncomment these lines:
        // Serial.print("Received: 0x");
        // Serial.print(c, HEX);
        
        switch (gvretState) {
            case IDLE:
                if (c == 0xE7) { // Enter binary mode
                    gvret_binOutput = true;
                } else if (c == 0xF1) {
                    gvretState = GET_COMMAND;
                }
                break;
                
            case GET_COMMAND:
                switch (c) {
                    case PROTO_BUILD_CAN_FRAME:
                        gvretState = BUILD_CAN_FRAME;
                        gvretStep = 0;
                        break;
                    case PROTO_TIME_SYNC:
                        gvretState = IDLE;
                        gvretStep = 0;
                        now = micros();
                        buff[0] = 0xF1;
                        buff[1] = 1;
                        buff[2] = now & 0xFF;
                        buff[3] = (now >> 8) & 0xFF;
                        buff[4] = (now >> 16) & 0xFF;
                        buff[5] = (now >> 24) & 0xFF;
                        GVRET_PORT.write(buff, 6);
                        break;
                    case PROTO_DIG_INPUTS:
                    case PROTO_ANA_INPUTS:
                    case PROTO_SET_DIG_OUT:
                    case PROTO_SETUP_CANBUS:
                        gvretState = IDLE;
                        break;
                    case PROTO_GET_CANBUS_PARAMS:
                        buff[0] = 0xF1;
                        buff[1] = 6;
                        buff[2] = 0;
                        buff[3] = 0xFF;
                        buff[4] = 0;
                        buff[5] = 0;
                        buff[6] = 0;
                        buff[7] = 0;
                        buff[8] = 0xFF;
                        buff[9] = 0;
                        buff[10] = 0;
                        buff[11] = 0;
                        GVRET_PORT.write(buff, 12);
                        gvretState = IDLE;
                        break;
                    case PROTO_GET_DEV_INFO:
                        buff[0] = 0xF1;
                        buff[1] = 7;
                        buff[2] = CFG_BUILD_NUM & 0xFF;
                        buff[3] = (CFG_BUILD_NUM >> 8);
                        buff[4] = 0x20;
                        buff[5] = 0;
                        buff[6] = 0;
                        buff[7] = 0;
                        GVRET_PORT.write(buff, 8);
                        gvretState = IDLE;
                        break;
                    case PROTO_SET_SW_MODE:
                    case PROTO_SET_SYSTYPE:
                    case PROTO_ECHO_CAN_FRAME:
                        gvretState = IDLE;
                        break;
                    case PROTO_KEEPALIVE:
                        buff[0] = 0xF1;
                        buff[1] = 0x09;
                        buff[2] = 0xDE;
                        buff[3] = 0xAD;
                        GVRET_PORT.write(buff, 4);
                        gvretState = IDLE;
                        break;
                    case PROTO_GET_NUMBUSES:
                        buff[0] = 0xF1;
                        buff[1] = 12;
                        buff[2] = 3;
                        GVRET_PORT.write(buff, 3);
                        gvretState = IDLE;
                        break;
                    case PROTO_GET_EXT_BUSES:
                        buff[0] = 0xF1;
                        buff[1] = 13;
                        for (int u = 2; u < 17; u++) {
                            buff[u] = 0;
                        }
                        GVRET_PORT.write(buff, 18);
                        gvretStep = 0;
                        gvretState = IDLE;
                        break;
                    case PROTO_SET_EXT_BUSES:
                        gvretState = IDLE;
                        break;
                    case PROTO_BUILD_FD_FRAME:
                        // FD frames are not handled.
                        gvretState = IDLE;
                        break;
                    case PROTO_SETUP_FD:
                    case PROTO_GET_FD:
                        gvretState = IDLE;
                        break;
                    default:
                        break;
                }
                break;
                
            case BUILD_CAN_FRAME:
                // For simplicity, full frame-building via USB is not implemented.
                gvretStep++;
                if (gvretStep >= (6 + 8)) { // After receiving a full frame, reset state.
                    gvretState = IDLE;
                }
                break;
                
            default:
                gvretState = IDLE;
                break;
        }
    }
}

// sendFrameToUSB: Send a standard CAN frame to USB using the GVRET protocol.
inline void sendFrameToUSB(const CANMessage &msg, int busNum) {
    uint8_t buff[20];
    uint32_t now = micros();
    buff[0] = 0xF1;
    buff[1] = 0;
    buff[2] = now & 0xFF;
    buff[3] = (now >> 8) & 0xFF;
    buff[4] = (now >> 16) & 0xFF;
    buff[5] = (now >> 24) & 0xFF;
    buff[6] = msg.id & 0xFF;
    buff[7] = (msg.id >> 8) & 0xFF;
    buff[8] = (msg.id >> 16) & 0xFF;
    buff[9] = (msg.id >> 24) & 0xFF;
    buff[10] = (busNum << 4) + msg.len;
    for (int i = 0; i < msg.len; i++) {
        buff[11 + i] = msg.data[i];
    }
    buff[11 + msg.len] = 0;
    GVRET_PORT.write(buff, 12 + msg.len);
}

#endif // GVRET_H
