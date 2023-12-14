#include "can_packer.h"

void printBits(uint64_t value) {
    for (int i = 63; i >= 0; --i) {
        // Use bitwise AND to check the value of the current bit
        uint64_t mask = 1ULL << i;
        uint64_t bit = (value & mask) >> i;
        Serial.print(bit);
        
        // Optionally add a space for better readability
        if (i % 8 == 0) {
            Serial.print(' ');
        }
    }
    Serial.println();  // Print a newline at the end
}

float unpack(const CANMessage& msg, byte start_bit, byte bit_len, bool big_endian,  float scale, float offset)
{
    uint64_t pduData = 0;

    // Swap bytes if big-endian
    if (big_endian)
    {
        pduData |= static_cast<uint64_t>(msg.data[7]) << 56;
        pduData |= static_cast<uint64_t>(msg.data[6]) << 48;
        pduData |= static_cast<uint64_t>(msg.data[5]) << 40;
        pduData |= static_cast<uint64_t>(msg.data[4]) << 32;
        pduData |= static_cast<uint64_t>(msg.data[3]) << 24;
        pduData |= static_cast<uint64_t>(msg.data[2]) << 16;
        pduData |= static_cast<uint64_t>(msg.data[1]) << 8;
        pduData |= static_cast<uint64_t>(msg.data[0]);
    }
    else
    {
        pduData = msg.data64;
    }

    // Calculate the bit mask based on start_bit and bit_len
    uint64_t mask = ((1ULL << bit_len) - 1ULL) << start_bit;

    // Apply the bitmask to the 64-bit local variable
    uint64_t shiftedData = (pduData & mask) >> start_bit;

    // Multiply by scale and add an offset
    return shiftedData * scale + offset;
}

void pack(CANMessage &msg, float value, byte start_bit, byte bit_len, bool big_endian, float scale, float offset)
{
    // Convert the float value to a 64-bit integer using the given scale and offset
    uint64_t packedData = static_cast<uint64_t>((value - offset) / scale);

    // Shift the data to the appropriate position if big-endian
    packedData <<= start_bit;
    
    // Calculate the bit mask based on bit_len and start_bit
    uint64_t mask = ((1ULL << bit_len) - 1ULL) << start_bit;

    // Apply the bitmask to the packed data
    packedData &= mask;

    if (big_endian)
    {
        // Use bitwise OR to combine with existing data in msg
        msg.data[7] |= static_cast<uint8_t>((packedData >> 56) & 0xFF);
        msg.data[6] |= static_cast<uint8_t>((packedData >> 48) & 0xFF);
        msg.data[5] |= static_cast<uint8_t>((packedData >> 40) & 0xFF);
        msg.data[4] |= static_cast<uint8_t>((packedData >> 32) & 0xFF);
        msg.data[3] |= static_cast<uint8_t>((packedData >> 24) & 0xFF);
        msg.data[2] |= static_cast<uint8_t>((packedData >> 16) & 0xFF);
        msg.data[1] |= static_cast<uint8_t>((packedData >> 8) & 0xFF);
        msg.data[0] |= static_cast<uint8_t>(packedData & 0xFF);
    }
    else
    {
        // If little-endian, simply copy the 64-bit data
        msg.data64 |= packedData;
    }
}

void pack(CANMessage &msg, bool value, byte start_bit, byte bit_len, bool big_endian)
{
    // Convert the boolean value to a uint64_t (1 for true, 0 for false)
    uint64_t packedData = value ? 1ULL : 0ULL;

    // Shift the data to the appropriate position if big-endian
    packedData <<= start_bit;

    if (big_endian)
    {
        // Use bitwise OR to combine with existing data in msg
        msg.data[7] |= static_cast<uint8_t>((packedData >> 56) & 0xFF);
        msg.data[6] |= static_cast<uint8_t>((packedData >> 48) & 0xFF);
        msg.data[5] |= static_cast<uint8_t>((packedData >> 40) & 0xFF);
        msg.data[4] |= static_cast<uint8_t>((packedData >> 32) & 0xFF);
        msg.data[3] |= static_cast<uint8_t>((packedData >> 24) & 0xFF);
        msg.data[2] |= static_cast<uint8_t>((packedData >> 16) & 0xFF);
        msg.data[1] |= static_cast<uint8_t>((packedData >> 8) & 0xFF);
        msg.data[0] |= static_cast<uint8_t>(packedData & 0xFF);
    }
    else
    {
        // If little-endian, simply copy the 64-bit data
        msg.data64 |= packedData;
    }
}


bool unpack(const CANMessage& msg, byte start_bit, byte bit_len, bool big_endian)
{
    uint64_t pduData = 0;

    // Swap bytes if big-endian
    if (big_endian)
    {
        pduData |= static_cast<uint64_t>(msg.data[7]) << 56;
        pduData |= static_cast<uint64_t>(msg.data[6]) << 48;
        pduData |= static_cast<uint64_t>(msg.data[5]) << 40;
        pduData |= static_cast<uint64_t>(msg.data[4]) << 32;
        pduData |= static_cast<uint64_t>(msg.data[3]) << 24;
        pduData |= static_cast<uint64_t>(msg.data[2]) << 16;
        pduData |= static_cast<uint64_t>(msg.data[1]) << 8;
        pduData |= static_cast<uint64_t>(msg.data[0]);
    }
    else
    {
        pduData = msg.data64;
    }

    // Calculate the bit mask based on start_bit and bit_len
    uint64_t mask = ((1ULL << bit_len) - 1ULL) << start_bit;

    // Apply the bitmask to the 64-bit local variable
    uint64_t shiftedData = (pduData & mask) >> start_bit;

    // Multiply by scale and add an offset
    return (shiftedData > 0);
}