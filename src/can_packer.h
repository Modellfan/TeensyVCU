#ifndef CAN_PACKER_H
#define CAN_PACKER_H

#include <Arduino.h>
#include <ACAN_T4.h>


void printBits(uint64_t value);

float unpack(const CANMessage& msg, byte start_bit, byte bit_len, bool big_endian, float scale, float offset);

void pack(CANMessage &msg, float value, byte start_bit, byte bit_len, bool big_endian, float scale, float offset);

void pack(CANMessage &msg, bool value, byte start_bit, byte bit_len, bool big_endian);

bool unpack(const CANMessage& msg, byte start_bit, byte bit_len, bool big_endian);

#endif  // CAN_FUNCTIONS_H
