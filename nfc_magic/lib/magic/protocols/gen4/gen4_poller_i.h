#pragma once

#include "gen4_poller.h"
#include <nfc/nfc_poller.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GEN4_POLLER_MAX_BUFFER_SIZE (64U)
#define GEN4_POLLER_MAX_FWT (60000U)

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

    Gen4PollerEvent gen4_event;
    Gen4PollerEventData gen4_event_data;

    Gen4PollerCallback callback;
    void* context;
};

#ifdef __cplusplus
}
#endif
