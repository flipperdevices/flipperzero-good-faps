#pragma once

#include "picopass_listener.h"

typedef enum {
    PicopassListenerStateIdle,
    PicopassListenerStateActive,
    PicopassListenerStateSelected,
} PicopassListenerState;

struct PicopassListener {
    Nfc* nfc;
    PicopassData* data;
    PicopassListenerState state;

    PicopassListenerEvent event;
    PicopassListenerCallback callback;
    void* context;
};
