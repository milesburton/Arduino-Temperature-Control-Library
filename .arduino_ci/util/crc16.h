#ifndef CRC16_H
#define CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

// Pure C implementation to replace the ASM version
static inline uint16_t _crc16_update(uint16_t crc, uint8_t data) {
    unsigned int i;
    crc ^= data;
    for (i = 0; i < 8; ++i) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    return crc;
}

#ifdef __cplusplus
}
#endif

#endif
