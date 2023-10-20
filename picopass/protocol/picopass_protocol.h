#pragma once

#include <flipper_format/flipper_format.h>

#ifdef __cplusplus
extern "C" {
#endif

// #define PICOPASS_FDT_LISTEN_FC (5000)
#define PICOPASS_FDT_LISTEN_FC (1000)

#define PICOPASS_LOCLASS_NUM_CSNS 9
#ifndef PICOPASS_LOCLASS_NUM_PER_CSN
// Collect 2 MACs per CSN to account for keyroll modes by default
#define PICOPASS_LOCLASS_NUM_PER_CSN 2
#endif
#define PICOPASS_LOCLASS_MACS_TO_COLLECT (PICOPASS_LOCLASS_NUM_CSNS * PICOPASS_LOCLASS_NUM_PER_CSN)

#define PICOPASS_BLOCK_LEN 8
#define PICOPASS_MAX_APP_LIMIT 32
#define PICOPASS_UID_LEN 8
#define PICOPASS_READ_CHECK_RESP_LEN 8
#define PICOPASS_CHECK_RESP_LEN 4
#define PICOPASS_MAC_LEN 4
#define PICOPASS_KEY_LEN 8

#define PICOPASS_CSN_BLOCK_INDEX 0
#define PICOPASS_CONFIG_BLOCK_INDEX 1
// These definitions for blocks above 2 only hold for secure cards.
#define PICOPASS_SECURE_EPURSE_BLOCK_INDEX 2
#define PICOPASS_SECURE_KD_BLOCK_INDEX 3
#define PICOPASS_SECURE_KC_BLOCK_INDEX 4
#define PICOPASS_SECURE_AIA_BLOCK_INDEX 5

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

// Personalization Mode
#define PICOPASS_FUSE_PERS 0x80
// Crypt1 // 1+1 (crypt1+crypt0) means secured and keys changable
#define PICOPASS_FUSE_CRYPT1 0x10
// Crypt0 // 1+0 means secure and keys locked, 0+1 means not secured, 0+0 means disable auth entirely
#define PICOPASS_FUSE_CRTPT0 0x08
#define PICOPASS_FUSE_CRYPT10 (PICOPASS_FUSE_CRYPT1 | PICOPASS_FUSE_CRTPT0)
// Read Access, 1 meanns anonymous read enabled, 0 means must auth to read applicaion
#define PICOPASS_FUSE_RA 0x01

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

typedef struct {
    uint8_t data[PICOPASS_BLOCK_LEN];
} PicopassBlock;

typedef struct {
    uint8_t data[PICOPASS_READ_CHECK_RESP_LEN];
} PicopassReadCheckResp;

typedef struct {
    uint8_t data[PICOPASS_CHECK_RESP_LEN];
} PicopassCheckResp;

typedef struct {
    uint8_t data[PICOPASS_MAC_LEN];
} PicopassMac;

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
    uint8_t key[PICOPASS_KEY_LEN];
    bool elite_kdf;
    uint8_t pin_length;
    PicopassEncryption encryption;
    uint8_t bitLength;
    uint8_t credential[8];
    uint8_t pin0[8];
    uint8_t pin1[8];
} PicopassPacs;

typedef struct {
    PicopassBlock AA1[PICOPASS_MAX_APP_LIMIT];
    PicopassPacs pacs;
} PicopassData;

PicopassData* picopass_protocol_alloc();

void picopass_protocol_free(PicopassData* instance);

void picopass_protocol_copy(PicopassData* data, const PicopassData* other);

void picopass_protocol_reset(PicopassData* instance);

bool picopass_protocol_save(const PicopassData* instance, FlipperFormat* ff);

bool picopass_protocol_save_as_lfrfid(const PicopassData* instance, const char* path);

bool picopass_protocol_load(PicopassData* instance, FlipperFormat* ff, uint32_t version);

void picopass_protocol_parse_credential(PicopassData* instance);

void picopass_protocol_parse_wiegand(PicopassData* instance);

#ifdef __cplusplus
}
#endif
