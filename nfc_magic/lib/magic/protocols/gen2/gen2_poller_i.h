#pragma once

#include "gen2_poller.h"
#include <nfc/protocols/nfc_generic_event.h>
#include "crypto1.h" // TODO: Move to a better home
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <lib/nfc/protocols/iso14443_3a/iso14443_3a_poller_i.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GEN2_POLLER_MAX_BUFFER_SIZE (64U)
#define GEN2_POLLER_MAX_FWT (60000U)

typedef enum {
    Gen2PollerErrorNone,
    Gen2PollerErrorNotPresent,
    Gen2PollerErrorProtocol,
    Gen2PollerErrorAuth,
    Gen2PollerErrorTimeout,
    Gen2PollerErrorAccess,
} Gen2PollerError;

typedef enum {
    Gen2PollerStateIdle,
    Gen2PollerStateRequestMode,
    Gen2PollerStateWipe,
    Gen2PollerStateWriteSourceDataRequest,
    Gen2PollerStateWriteTargetDataRequest,
    Gen2PollerStateWrite,
    Gen2PollerStateSuccess,
    Gen2PollerStateFail,

    Gen2PollerStateNum,
} Gen2PollerState;

typedef enum {
    Gen2PollerSessionStateIdle,
    Gen2PollerSessionStateStarted,
    Gen2PollerSessionStateStopRequest,
} Gen2PollerSessionState;

typedef enum {
    Gen2AuthStateIdle,
    Gen2AuthStatePassed,
} Gen2AuthState;

typedef enum {
    Gen2CardStateDetected,
    Gen2CardStateLost,
} Gen2CardState;

typedef struct {
    MfClassicData* mfc_data_source;
    MfClassicData* mfc_data_target;
    MfClassicKey auth_key;
    MfClassicKeyType read_key;
    MfClassicKeyType write_key;
    uint16_t current_block;
    bool need_halt_before_write;
} Gen2PollerWriteContext;

typedef union {
    Gen2PollerWriteContext write_ctx;
} Gen2PollerModeContext;

struct Gen2Poller {
    Nfc* nfc;
    Gen2PollerState state;
    Gen2PollerSessionState session_state;

    NfcPoller* poller;
    Iso14443_3aPoller* iso3_poller;

    Gen2AuthState auth_state;
    Gen2CardState card_state;

    uint8_t sectors_total;
    Gen2PollerModeContext mode_ctx;

    Crypto1* crypto;
    BitBuffer* tx_plain_buffer;
    BitBuffer* tx_encrypted_buffer;
    BitBuffer* rx_plain_buffer;
    BitBuffer* rx_encrypted_buffer;
    MfClassicData* data;

    Gen2PollerEvent gen2_event;
    Gen2PollerEventData gen2_event_data;

    Gen2PollerCallback callback;
    void* context;
};

typedef struct {
    uint8_t block;
    MfClassicKeyType key_type;
    MfClassicNt nt;
} Gen2CollectNtContext;

typedef struct {
    uint8_t block_num;
    MfClassicKey key;
    MfClassicKeyType key_type;
    MfClassicBlock block;
} Gen2ReadBlockContext;

typedef struct {
    uint8_t block_num;
    MfClassicKey key;
    MfClassicKeyType key_type;
    MfClassicBlock block;
} Gen2WriteBlockContext;

Gen2PollerError gen2_poller_write(Gen2Poller* instance);

Gen2PollerError gen2_poller_auth(
    Gen2Poller* instance,
    uint8_t block_num,
    MfClassicKey* key,
    MfClassicKeyType key_type,
    MfClassicAuthContext* data);

Gen2PollerError gen2_poller_halt(Gen2Poller* instance);

Gen2PollerError
    gen2_poller_write_block(Gen2Poller* instance, uint8_t block_num, const MfClassicBlock* data);

MfClassicKeyType
    gen2_poller_get_key_type_to_write(const MfClassicData* mfc_data, uint8_t block_num);

bool gen2_poller_can_write_block(const MfClassicData* mfc_data, uint8_t block_num);

bool gen2_can_reset_access_conditions(const MfClassicData* mfc_data, uint8_t block_num);

bool gen2_poller_can_write_data_block(const MfClassicData* mfc_data, uint8_t block_num);

bool gen2_poller_can_write_sector_trailer(const MfClassicData* mfc_data, uint8_t block_num);

bool gen2_is_allowed_access(
    const MfClassicData* data,
    uint8_t block_num,
    MfClassicKeyType key_type,
    MfClassicAction action);

#ifdef __cplusplus
}
#endif