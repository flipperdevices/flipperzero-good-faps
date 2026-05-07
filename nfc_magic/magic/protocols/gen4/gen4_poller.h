#pragma once

#include "gen4.h"
#include <nfc/nfc.h>
#include <nfc/protocols/nfc_protocol.h>
#include <nfc/protocols/mf_classic/mf_classic.h>
#include <nfc/protocols/mf_ultralight/mf_ultralight.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO: cleanup, check gen4_poller_i.c defines
#define GEN4_CMD_PREFIX       (0xCF)
#define GEN4_CMD_GET_CFG      (0xC6)
#define GEN4_CMD_GET_REVISION (0xCC)
#define GEN4_CMD_WRITE        (0xCD)
#define GEN4_CMD_READ         (0xCE)
#define GEN4_CMD_SET_CFG      (0xF0)
#define GEN4_CMD_FUSE_CFG     (0xF1)
#define GEN4_CMD_SET_PWD      (0xFE)

typedef enum {
    Gen4PollerErrorNone,
    Gen4PollerErrorTimeout,
    Gen4PollerErrorProtocol,
} Gen4PollerError;

typedef enum {
    Gen4PollerEventTypeCardDetected,
    Gen4PollerEventTypeRequestMode,
    Gen4PollerEventTypeRequestDataToWrite,
    Gen4PollerEventTypeRequestNewPassword,

    Gen4PollerEventTypeSuccess,
    Gen4PollerEventTypeFail,
} Gen4PollerEventType;

typedef enum {
    Gen4PollerModeWipe,
    Gen4PollerModeWrite,
    Gen4PollerModeSetPassword,
    Gen4PollerModeGetInfo,
    Gen4PollerModeSetDefaultCfg,
    Gen4PollerModeSetShadowMode,
    Gen4PollerModeSetDirectWriteBlock0Mode,
    Gen4PollerModeWriteUID,
    Gen4PollerModeSetATQASAK,
    Gen4PollerModeSetATS,
    Gen4PollerModeSetULProtocol,
    Gen4PollerModeSetULMode,
    Gen4PollerModeSetMaxRWBlock,
    Gen4PollerModeSetType,
    Gen4PollerModeWriteNTAGPwd,
    Gen4PollerModeWritePack,
    Gen4PollerModeWriteOTP,
    Gen4PollerModeWriteVersion,
    Gen4PollerModeWriteSignature,
    Gen4PollerModeFullWipe,
} Gen4PollerMode;

typedef struct {
    Gen4PollerMode mode;
} Gen4PollerEventDataRequestMode;

typedef struct {
    NfcProtocol protocol;
    const NfcDeviceData* data;
} Gen4PollerEventDataRequestDataToWrite;

typedef struct {
    Gen4Password password;
} Gen4PollerEventDataRequestNewPassword;

typedef union {
    Gen4PollerEventDataRequestMode request_mode;
    Gen4PollerEventDataRequestDataToWrite request_data;
    Gen4PollerEventDataRequestNewPassword request_password;
} Gen4PollerEventData;

typedef struct {
    Gen4PollerEventType type;
    Gen4PollerEventData* data;
} Gen4PollerEvent;

typedef NfcCommand (*Gen4PollerCallback)(Gen4PollerEvent event, void* context);

typedef struct Gen4Poller Gen4Poller;

Gen4PollerError gen4_poller_detect(Nfc* nfc, Gen4Password password, Gen4* gen4_data);

Gen4Poller* gen4_poller_alloc(Nfc* nfc);

void gen4_poller_free(Gen4Poller* instance);

void gen4_poller_set_password(Gen4Poller* instance, Gen4Password password);

void gen4_poller_start(Gen4Poller* instance, Gen4PollerCallback callback, void* context);

void gen4_poller_stop(Gen4Poller* instance);

const Gen4* gen4_poller_get_gen4_data(const Gen4Poller* instance);

void gen4_poller_struct_set_direct_write_block_0_mode(
    Gen4Poller* instance,
    Gen4DirectWriteBlock0Mode mode);

void gen4_poller_struct_set_shadow_mode(Gen4Poller* instance, Gen4ShadowMode mode);

void gen4_poller_struct_set_uid(Gen4Poller* instance, const uint8_t* uid, uint8_t uid_len);
void gen4_poller_struct_set_atqa_sak_params(
    Gen4Poller* instance,
    uint8_t atqa0,
    uint8_t atqa1,
    uint8_t sak);
void gen4_poller_struct_set_ats_params(Gen4Poller* instance, const uint8_t* ats, uint8_t ats_len);
void gen4_poller_struct_set_protocol(Gen4Poller* instance, Gen4Protocol protocol);
void gen4_poller_struct_set_ul_mode(Gen4Poller* instance, Gen4UltralightMode mode);
void gen4_poller_struct_set_max_rw_block(Gen4Poller* instance, uint8_t max_block);
void gen4_poller_struct_set_tag_type(Gen4Poller* instance, uint8_t tag_type);
void gen4_poller_struct_set_ntag_pwd(Gen4Poller* instance, const uint8_t* pwd);
void gen4_poller_struct_set_pack(Gen4Poller* instance, const uint8_t* pack);
void gen4_poller_struct_set_otp(Gen4Poller* instance, const uint8_t* otp);
void gen4_poller_struct_set_version(Gen4Poller* instance, const uint8_t* version);
void gen4_poller_struct_set_signature(Gen4Poller* instance, const uint8_t* signature);
void gen4_poller_struct_set_wipe_type(Gen4Poller* instance, uint8_t wipe_type);

#ifdef __cplusplus
}
#endif
