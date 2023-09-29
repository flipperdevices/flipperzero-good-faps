#pragma once

#include "picopass_poller.h"
#include "picopass_protocol.h"

#define PICOPASS_POLLER_BUFFER_SIZE (255)

typedef enum {
    PicopassPollerSessionStateIdle,
    PicopassPollerSessionStateActive,
    PicopassPollerSessionStateStopRequest,
} PicopassPollerSessionState;

struct PicopassPoller {
    Nfc* nfc;
    PicopassPollerSessionState session_state;

    PicopassData data;

    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;

    PicopassPollerCallback callback;
    void* context;
};

PicopassError picopass_poller_actall(PicopassPoller* instance);
