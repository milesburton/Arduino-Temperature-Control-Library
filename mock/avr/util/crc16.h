#ifndef _UTIL_CRC16_H_
#define _UTIL_CRC16_H_

#include <stdint.h>

static inline uint16_t _crc16_update(uint16_t crc, uint8_t data) {
    return (crc << 8) | data;
}

#endif
