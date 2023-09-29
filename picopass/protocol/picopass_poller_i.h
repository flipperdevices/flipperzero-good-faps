#pragma once

#include "picopass_poller.h"

#define PICOPASS_POLLER_BUFFER_SIZE (255)

typedef enum {
    PicopassPollerSessionStateIdle,
    PicopassPollerSessionStateActive,
    PicopassPollerSessionStateStopRequest,
} PicopassPollerSessionState;

struct PicopassPoller {
    Nfc* nfc;
    PicopassPollerSessionState session_state;

    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;

    PicopassPollerCallback callback;
    void* context;
};
