#pragma once

#include <nfc/nfc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PicopassPollerEventTypeSuccess,
    PicopassPollerEventTypeFail,
} PicopassPollerEventType;

typedef struct {
    PicopassPollerEventType type;
} PicopassPollerEvent;

typedef NfcCommand (*PicopassPollerCallback)(PicopassPollerEvent event, void* context);

typedef struct PicopassPoller PicopassPoller;

PicopassPoller* picopass_poller_alloc(Nfc* nfc);

void picopass_poller_free(PicopassPoller* instance);

void picopass_poller_start(
    PicopassPoller* instance,
    PicopassPollerCallback callback,
    void* context);

void picopass_poller_stop(PicopassPoller* instance);

#ifdef __cplusplus
}
#endif
