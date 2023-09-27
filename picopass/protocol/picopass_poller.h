#pragma once

#include <nfc/nfc.h>

typedef struct PicopassPoller PicopassPoller;

PicopassPoller* picopass_poller_alloc(Nfc* nfc);

void picopass_poller_free(PicopassPoller* instance);

void picopass_poller_start(PicopassPoller* instance);

void picopass_poller_stop(PicopassPoller* instance);
