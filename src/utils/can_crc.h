#ifndef CAN_CRC_H
#define CAN_CRC_H

#include <stdint.h>

static inline uint32_t crc32_word(uint32_t Crc, uint32_t Data)
{
    Crc ^= Data;
    for (int i = 0; i < 32; i++) {
        if (Crc & 0x80000000U)
            Crc = (Crc << 1) ^ 0x04C11DB7U;
        else
            Crc <<= 1;
    }
    return Crc;
}

static inline uint32_t crc32(const uint32_t *data, uint32_t len, uint32_t crc)
{
    for (uint32_t i = 0; i < len; i++)
        crc = crc32_word(crc, data[i]);
    return crc;
}

static inline uint8_t can_crc8(const uint8_t *bytes)
{
    uint32_t buf[2];
    buf[0] = ((uint32_t)bytes[3] << 24) | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[1] << 8) | bytes[0];
    buf[1] = ((uint32_t)bytes[7] << 24) | ((uint32_t)bytes[6] << 16) | ((uint32_t)bytes[5] << 8) | bytes[4];
    uint32_t crc = crc32(buf, 2, 0xFFFFFFFFU);
    return (uint8_t)(crc & 0xFFU);
}

#endif // CAN_CRC_H
