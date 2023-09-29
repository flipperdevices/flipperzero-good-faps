#include "picopass_poller_i.h"

#define TAG "Picopass"

static PicopassError picopass_poller_process_error(NfcError error) {
    PicopassError ret = PicopassErrorNone;

    switch(error) {
    case NfcErrorNone:
        ret = PicopassErrorNone;
        break;

    default:
        ret = PicopassErrorTimeout;
        break;
    }

    return ret;
}

PicopassError picopass_poller_actall(PicopassPoller* instance) {
    PicopassError ret = PicopassErrorNone;

    bit_buffer_reset(instance->tx_buffer);
    bit_buffer_append_byte(instance->tx_buffer, PICOPASS_CMD_ACTALL);

    NfcError error =
        nfc_poller_trx(instance->nfc, instance->tx_buffer, instance->rx_buffer, 100000);
    if(error != NfcErrorIncompleteFrame) {
        ret = picopass_poller_process_error(error);
    }

    return ret;
}
