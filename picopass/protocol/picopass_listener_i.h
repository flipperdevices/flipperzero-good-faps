#pragma once

#include "picopass_listener.h"
#include <nfc/helpers/iso13239_crc.h>

#define TAG "PicopassListener"

#define PICOPASS_LISTENER_BUFFER_SIZE_MAX (255)

typedef enum {
    PicopassListenerStateIdle,
    PicopassListenerStateHalt,
    PicopassListenerStateActive,
    PicopassListenerStateSelected,
} PicopassListenerState;

struct PicopassListener {
    Nfc* nfc;
    PicopassData* data;
    PicopassListenerState state;

    BitBuffer* tx_buffer;
    BitBuffer* tmp_buffer;

    PicopassListenerEvent event;
    PicopassListenerCallback callback;
    void* context;
};

PicopassError picopass_listener_send_frame(PicopassListener* instance, BitBuffer* tx_buffer);

PicopassError picopass_listener_write_anticoll_csn(PicopassListener* instance, BitBuffer* buffer);
