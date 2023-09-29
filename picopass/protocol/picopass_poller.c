#include "picopass_poller_i.h"

#include <furi/furi.h>

#define TAG "Picopass"

PicopassPoller* picopass_poller_alloc(Nfc* nfc) {
    furi_assert(nfc);

    PicopassPoller* instance = malloc(sizeof(PicopassPoller));
    instance->nfc = nfc;
    nfc_config(instance->nfc, NfcModePoller, NfcTechIso15693);
    nfc_set_guard_time_us(instance->nfc, 10000);
    nfc_set_fdt_poll_fc(instance->nfc, 5000);
    nfc_set_fdt_poll_poll_us(instance->nfc, 1000);

    instance->tx_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);
    instance->rx_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);

    return instance;
}

void picopass_poller_free(PicopassPoller* instance) {
    furi_assert(instance);

    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);
    free(instance);
}

static NfcCommand picopass_poller_callback(NfcEvent event, void* context) {
    furi_assert(context);

    PicopassPoller* instance = context;
    NfcCommand command = NfcCommandContinue;

    if(event.type == NfcEventTypePollerReady) {
        PicopassError error = picopass_poller_actall(instance);
        FURI_LOG_D(TAG, "Error: %d", error);
        command = NfcCommandReset;
    }

    if(instance->session_state == PicopassPollerSessionStateStopRequest) {
        command = NfcCommandStop;
    }

    return command;
}

void picopass_poller_start(
    PicopassPoller* instance,
    PicopassPollerCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(instance->session_state == PicopassPollerSessionStateIdle);

    instance->callback = callback;
    instance->context = context;

    instance->session_state = PicopassPollerSessionStateActive;
    nfc_start(instance->nfc, picopass_poller_callback, instance);
}

void picopass_poller_stop(PicopassPoller* instance) {
    furi_assert(instance);

    instance->session_state = PicopassPollerSessionStateStopRequest;
    nfc_stop(instance->nfc);
    instance->session_state = PicopassPollerSessionStateIdle;
}
