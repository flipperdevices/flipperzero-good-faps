#pragma once

#include "gen4_poller.h"
#include <nfc/nfc_poller.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <bit_lib/bit_lib.h>

#define TAG "Gen4Poller"

#ifdef __cplusplus
extern "C" {
#endif

#define GEN4_POLLER_MAX_BUFFER_SIZE (64U)
#define GEN4_POLLER_MAX_FWT         (200000U)

#define GEN4_POLLER_BLOCK_SIZE   (16)
#define GEN4_POLLER_BLOCKS_TOTAL (256)

typedef enum {
    Gen4PollerStateIdle,
    Gen4PollerStateRequestMode,
    Gen4PollerStateRequestWriteData,
    Gen4PollerStateWrite,
    Gen4PollerStateWipe,
    Gen4PollerStateChangePassword,
    Gen4PollerStateGetInfo,
    Gen4PollerStateSetDefaultConfig,
    Gen4PollerStateSetShadowMode,
    Gen4PollerStateSetDirectWriteBlock0,
    Gen4PollerStateWriteUID,
    Gen4PollerStateSetATQASAK,
    Gen4PollerStateSetATS,
    Gen4PollerStateSetULProtocol,
    Gen4PollerStateSetULMode,
    Gen4PollerStateSetMaxRWBlock,
    Gen4PollerStateSetType,
    Gen4PollerStateWriteNTAGPwd,
    Gen4PollerStateWritePack,
    Gen4PollerStateWriteOTP,
    Gen4PollerStateWriteVersion,
    Gen4PollerStateWriteSignature,
    Gen4PollerStateFullWipe,
    Gen4PollerStateSuccess,
    Gen4PollerStateFail,
    Gen4PollerStateNum,
} Gen4PollerState;

struct Gen4Poller {
    NfcPoller* poller;
    Iso14443_3aPoller* iso3_poller;
    Gen4PollerState state;

    Gen4* gen4_data;

    Gen4Password password;

    Gen4Password new_password;
    Gen4Config config;
    Gen4ShadowMode shadow_mode;
    Gen4DirectWriteBlock0Mode direct_write_block_0_mode;

    uint8_t uid[GEN4_UID_MAX_LEN];
    uint8_t uid_len;
    uint8_t atqa0;
    uint8_t atqa1;
    uint8_t sak;
    uint8_t ats[GEN4_ATS_MAX_LEN];
    uint8_t ats_len;
    Gen4Protocol ul_protocol;
    Gen4UltralightMode ul_mode;
    uint8_t max_rw_block;
    uint8_t tag_type;
    uint8_t ntag_pwd[4];
    uint8_t pack[2];
    uint8_t otp[4];
    uint8_t version[8];
    uint8_t signature[GEN4_SIGNATURE_SIZE];
    uint8_t wipe_type;
    bool has_ul_auth;

    uint8_t cmd_step;

    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;

    uint16_t current_block;
    uint16_t total_blocks;

    NfcProtocol protocol;
    const NfcDeviceData* data;

    Gen4PollerEvent gen4_event;
    Gen4PollerEventData gen4_event_data;

    Gen4PollerCallback callback;
    void* context;
};

Gen4PollerError gen4_poller_set_config(
    Gen4Poller* instance,
    Gen4Password password,
    const Gen4Config* config,
    size_t config_size,
    bool fuse);

Gen4PollerError gen4_poller_write_block(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t block_num,
    const uint8_t* data);

Gen4PollerError gen4_poller_change_password(
    Gen4Poller* instance,
    Gen4Password pwd_current,
    Gen4Password pwd_new);

Gen4PollerError gen4_poller_get_revision(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4Revision* revision_result);

Gen4PollerError
    gen4_poller_get_config(Gen4Poller* instance, Gen4Password password, Gen4Config* config_result);

Gen4PollerError
    gen4_poller_set_shadow_mode(Gen4Poller* instance, Gen4Password password, Gen4ShadowMode mode);

Gen4PollerError gen4_poller_set_direct_write_block_0_mode(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4DirectWriteBlock0Mode mode);

Gen4PollerError gen4_poller_set_ats(
    Gen4Poller* instance,
    Gen4Password password,
    const uint8_t* ats_data,
    uint8_t ats_len);

Gen4PollerError gen4_poller_set_atqa_sak(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t atqa0,
    uint8_t atqa1,
    uint8_t sak);

Gen4PollerError gen4_poller_set_ul_protocol(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4Protocol protocol);

Gen4PollerError gen4_poller_set_ul_mode(
    Gen4Poller* instance,
    Gen4Password password,
    Gen4UltralightMode mode);

Gen4PollerError gen4_poller_set_max_rw_block(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t max_block);

Gen4PollerError gen4_poller_read_memory(
    Gen4Poller* instance,
    Gen4Password password,
    uint8_t block_num,
    uint8_t* data,
    uint8_t data_len);

Gen4PollerError gen4_poller_mfu_read_block(
    Gen4Poller* instance,
    uint8_t block_num,
    uint8_t* data);

Gen4PollerError gen4_poller_mfu_write_block(
    Gen4Poller* instance,
    uint8_t block_num,
    const uint8_t* data);

Gen4PollerError gen4_poller_mfu_auth_pwd(
    Gen4Poller* instance,
    const uint8_t* password);

#ifdef __cplusplus
}
#endif
