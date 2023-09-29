#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PICOPASS_BLOCK_LEN 8
#define PICOPASS_MAX_APP_LIMIT 32

typedef enum {
    PicopassDeviceEncryptionUnknown = 0,
    PicopassDeviceEncryptionNone = 0x14,
    PicopassDeviceEncryptionDES = 0x15,
    PicopassDeviceEncryption3DES = 0x17,
} PicopassEncryption;

typedef struct {
    bool legacy;
    bool se_enabled;
    bool sio;
    bool biometrics;
    uint8_t key[8];
    bool elite_kdf;
    uint8_t pin_length;
    PicopassEncryption encryption;
    uint8_t bitLength;
    uint8_t credential[8];
    uint8_t pin0[8];
    uint8_t pin1[8];
} PicopassPacs;

typedef struct {
    uint8_t data[PICOPASS_BLOCK_LEN];
} PicopassBlock;

typedef struct {
    PicopassBlock AA1[PICOPASS_MAX_APP_LIMIT];
    PicopassPacs pacs;
} PicopassData;

#ifdef __cplusplus
}
#endif
