#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PICOPASS_BLOCK_LEN 8
#define PICOPASS_MAX_APP_LIMIT 32
#define PICOPASS_UID_LEN 8

#define PICOPASS_CMD_READ_OR_IDENTIFY (0x0C)
// ADDRESS(1) CRC16(2) -> DATA(32) CRC16(2)
#define PICOPASS_CMD_READ4 (0x06)
// ADDRESS(1) DATA(8) SIGN(4)|CRC16(2) -> DATA(8) CRC16(2)
#define PICOPASS_CMD_UPDATE (0x87)
// ADDRESS(1) -> DATA(8)
#define PICOPASS_CMD_READCHECK_KD (0x88)
// ADDRESS(1) -> DATA(8)
#define PICOPASS_CMD_READCHECK_KC (0x18)
// CHALLENGE(4) READERSIGNATURE(4) -> CHIPRESPONSE(4)
#define PICOPASS_CMD_CHECK (0x05)
// No args -> SOF
#define PICOPASS_CMD_ACTALL (0x0A)
// No args -> SOF
#define PICOPASS_CMD_ACT (0x8E)
// ASNB(8)|SERIALNB(8) -> SERIALNB(8) CRC16(2)
#define PICOPASS_CMD_SELECT (0x81)
// No args -> SERIALNB(8) CRC16(2)
#define PICOPASS_CMD_DETECT (0x0F)
// No args -> SOF
#define PICOPASS_CMD_HALT (0x00)
// PAGE(1) CRC16(2) -> BLOCK1(8) CRC16(2)
#define PICOPASS_CMD_PAGESEL (0x84)

typedef enum {
    PicopassErrorNone,
    PicopassErrorTimeout,
    PicopassErrorIncorrectCrc,
    PicopassErrorProtocol,
} PicopassError;

typedef struct {
    uint8_t data[PICOPASS_UID_LEN];
} PicopassColResSerialNum;

typedef struct {
    uint8_t data[PICOPASS_UID_LEN];
} PicopassSerialNum;

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
