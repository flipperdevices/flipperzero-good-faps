#pragma once

#include <nfc/nfc.h>
#include "picopass_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PicopassListenerEventTypeActivated,
} PicopassListenerEventType;

typedef struct {
    PicopassListenerEventType type;
} PicopassListenerEvent;

typedef NfcCommand (*PicopassListenerCallback)(PicopassListenerEvent event, void* context);

typedef struct PicopassListener PicopassListener;

PicopassListener* picopass_listener_alloc(Nfc* nfc, const PicopassData* data);

void picopass_listener_free(PicopassListener* instance);

void picopass_listener_start(
    PicopassListener* instance,
    PicopassListenerCallback callback,
    void* context);

void picopass_listener_stop(PicopassListener* instance);

const PicopassData* picopass_listener_get_data(PicopassListener* instance);

#ifdef __cplusplus
}
#endif
