#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <mbedtls/des.h>

#include "rfal_picopass.h"
#include "loclass_writer.h"
#include <optimized_ikeys.h>
#include <optimized_cipher.h>
#include "helpers/iclass_elite_dict.h"

#define LOCLASS_NUM_CSNS 9
#ifndef LOCLASS_NUM_PER_CSN
// Collect 2 MACs per CSN to account for keyroll modes by default
#define LOCLASS_NUM_PER_CSN 2
#endif
#define LOCLASS_MACS_TO_COLLECT (LOCLASS_NUM_CSNS * LOCLASS_NUM_PER_CSN)

#define PICOPASS_DEV_NAME_MAX_LEN 22
#define PICOPASS_READER_DATA_MAX_SIZE 64
#define PICOPASS_MAX_APP_LIMIT 32

#define PICOPASS_CSN_BLOCK_INDEX 0
#define PICOPASS_CONFIG_BLOCK_INDEX 1
// These definitions for blocks above 2 only hold for secure cards.
#define PICOPASS_SECURE_EPURSE_BLOCK_INDEX 2
#define PICOPASS_SECURE_KD_BLOCK_INDEX 3
#define PICOPASS_SECURE_KC_BLOCK_INDEX 4
#define PICOPASS_SECURE_AIA_BLOCK_INDEX 5
// Non-secure cards instead have an AIA at block 2
#define PICOPASS_NONSECURE_AIA_BLOCK_INDEX 2
// Only iClass cards
#define PICOPASS_ICLASS_PACS_CFG_BLOCK_INDEX 6

// Personalization Mode
#define PICOPASS_FUSE_PERS 0x80
// Crypt1 // 1+1 (crypt1+crypt0) means secured and keys changable
#define PICOPASS_FUSE_CRYPT1 0x10
// Crypt0 // 1+0 means secure and keys locked, 0+1 means not secured, 0+0 means disable auth entirely
#define PICOPASS_FUSE_CRTPT0 0x08
#define PICOPASS_FUSE_CRYPT10 (PICOPASS_FUSE_CRYPT1 | PICOPASS_FUSE_CRTPT0)
// Read Access, 1 meanns anonymous read enabled, 0 means must auth to read applicaion
#define PICOPASS_FUSE_RA 0x01

#define PICOPASS_APP_FOLDER ANY_PATH("picopass")
#define PICOPASS_APP_EXTENSION ".picopass"
#define PICOPASS_APP_FILE_PREFIX "Picopass"
#define PICOPASS_APP_SHADOW_EXTENSION ".pas"

#define PICOPASS_DICT_KEY_BATCH_SIZE 10

typedef void (*PicopassLoadingCallback)(void* context, bool state);

typedef struct {
    IclassEliteDict* dict;
    IclassEliteDictType type;
    uint8_t current_sector;
} IclassEliteDictAttackData;

typedef enum {
    PicopassDeviceEncryptionUnknown = 0,
    PicopassDeviceEncryptionNone = 0x14,
    PicopassDeviceEncryptionDES = 0x15,
    PicopassDeviceEncryption3DES = 0x17,
} PicopassEncryption;

typedef enum {
    PicopassDeviceSaveFormatHF,
    PicopassDeviceSaveFormatLF,
} PicopassDeviceSaveFormat;

typedef enum {
    PicopassEmulatorStateHalt,
    PicopassEmulatorStateIdle,
    PicopassEmulatorStateActive,
    PicopassEmulatorStateSelected,
    PicopassEmulatorStateStopEmulation,
} PicopassEmulatorState;

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
} PicopassDeviceData;

typedef struct {
    PicopassEmulatorState state;
    LoclassState_t cipher_state;
    uint8_t key_block_num; // in loclass mode used to store csn#
    bool loclass_mode;
    bool loclass_got_std_key;
    uint8_t loclass_mac_buffer[8 * LOCLASS_NUM_PER_CSN];
    LoclassWriter* loclass_writer;
} PicopassEmulatorCtx;

typedef struct {
    Storage* storage;
    DialogsApp* dialogs;
    PicopassDeviceData dev_data;
    char dev_name[PICOPASS_DEV_NAME_MAX_LEN + 1];
    FuriString* load_path;
    PicopassDeviceSaveFormat format;
    PicopassLoadingCallback loading_cb;
    void* loading_cb_ctx;
} PicopassDevice;

PicopassDevice* picopass_device_alloc();

void picopass_device_free(PicopassDevice* picopass_dev);

void picopass_device_set_name(PicopassDevice* dev, const char* name);

bool picopass_device_save(PicopassDevice* dev, const char* dev_name);

bool picopass_file_select(PicopassDevice* dev);

void picopass_device_data_clear(PicopassDeviceData* dev_data);

void picopass_device_clear(PicopassDevice* dev);

bool picopass_device_delete(PicopassDevice* dev, bool use_load_path);

void picopass_device_set_loading_callback(
    PicopassDevice* dev,
    PicopassLoadingCallback callback,
    void* context);

void picopass_device_parse_credential(PicopassBlock* AA1, PicopassPacs* pacs);
void picopass_device_parse_wiegand(uint8_t* credential, PicopassPacs* pacs);
bool picopass_device_hid_csn(PicopassDevice* dev);
