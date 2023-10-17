#include "picopass_listener_i.h"

#include <furi/furi.h>

static PicopassError picopass_listener_process_error(NfcError error) {
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

PicopassError picopass_listener_send_frame(PicopassListener* instance, BitBuffer* tx_buffer) {
    iso13239_crc_append(Iso13239CrcTypePicopass, tx_buffer);
    NfcError error = nfc_listener_tx(instance->nfc, tx_buffer);

    return picopass_listener_process_error(error);
}

// from proxmark3 armsrc/iclass.c rotateCSN
PicopassError picopass_listener_write_anticoll_csn(PicopassListener* instance, BitBuffer* buffer) {
    const uint8_t* uid = instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data;
    bit_buffer_reset(buffer);
    for(size_t i = 0; i < PICOPASS_BLOCK_LEN; i++) {
        bit_buffer_append_byte(buffer, (uid[i] >> 3) | (uid[(i + 1) % 8] << 5));
    }

    return PicopassErrorNone;
}
