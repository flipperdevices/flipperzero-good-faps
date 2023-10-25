#pragma once

#include "gen4_poller.h"
#include <nfc/nfc_poller.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>

#define TAG "Gen4Poller"

#ifdef __cplusplus
extern "C" {
#endif

#define GEN4_POLLER_MAX_BUFFER_SIZE (64U)
#define GEN4_POLLER_MAX_FWT (200000U)

#define GEN4_POLLER_BLOCK_SIZE (16)
#define GEN4_POLLER_BLOCKS_TOTAL (256)

typedef enum {
    Gen4PollerErrorNone,
    Gen4PollerErrorTimeout,
    Gen4PollerErrorProtocol,
} Gen4PollerError;

typedef enum {
    Gen4PollerStateIdle,
    Gen4PollerStateRequestMode,
    Gen4PollerStateWipe,
    Gen4PollerStateWriteDataRequest,
    Gen4PollerStateWrite,
    Gen4PollerStateSuccess,
    Gen4PollerStateFail,

    Gen4PollerStateNum,
} Gen4PollerState;

struct Gen4Poller {
    NfcPoller* poller;
    Iso14443_3aPoller* iso3_poller;
    Gen4PollerState state;
    uint32_t password;

    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;

    uint16_t current_block;

    Gen4PollerEvent gen4_event;
    Gen4PollerEventData gen4_event_data;

    Gen4PollerCallback callback;
    void* context;
};

Gen4PollerError gen4_poller_set_config(
    Gen4Poller* instance,
    uint32_t password,
    const uint8_t* config,
    size_t config_size,
    bool fuse);

Gen4PollerError gen4_poller_write_block(
    Gen4Poller* instance,
    uint32_t password,
    uint8_t block_num,
    const uint8_t* data);

#ifdef __cplusplus
}
#endif
