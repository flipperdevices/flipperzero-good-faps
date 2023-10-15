#include "picopass_listener_i.h"

#include <furi/furi.h>

typedef enum {
    PicopassListenerCommandProcessed,
    PicopassListenerCommandSilent,
    PicopassListenerCommandSendSoF,
} PicopassListenerCommand;

typedef PicopassListenerCommand (
    *PicopassListenerCommandHandler)(PicopassListener* instance, BitBuffer* buf);

typedef struct {
    uint8_t start_byte_cmd;
    size_t cmd_len_bits;
    PicopassListenerCommandHandler handler;
} PicopassListenerCmd;

PicopassListenerCommand picopass_listener_act_handler(PicopassListener* instance, BitBuffer* buf) {
    UNUSED(buf);

    instance->state = PicopassListenerStateActive;

    return PicopassListenerCommandSendSoF;
}

static const PicopassListenerCmd picopass_listener_cmd_handlers[] = {
    {
        .start_byte_cmd = PICOPASS_CMD_ACT,
        .cmd_len_bits = 8,
        .handler = picopass_listener_act_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_ACTALL,
        .cmd_len_bits = 8,
        .handler = picopass_listener_act_handler,
    },

};

PicopassListener* picopass_listener_alloc(Nfc* nfc, const PicopassData* data) {
    furi_assert(nfc);
    furi_assert(data);

    PicopassListener* instance = malloc(sizeof(PicopassListener));
    instance->nfc = nfc;
    instance->data = picopass_protocol_alloc();

    nfc_set_fdt_listen_fc(instance->nfc, PICOPASS_FDT_LISTEN_FC);
    nfc_config(instance->nfc, NfcModeListener, NfcTechIso15693);

    return instance;
}

void picopass_listener_free(PicopassListener* instance) {
    furi_assert(instance);

    picopass_protocol_free(instance->data);
    free(instance);
}

NfcCommand picopass_listener_start_callback(NfcEvent event, void* context) {
    furi_assert(context);

    NfcCommand command = NfcCommandContinue;
    PicopassListener* instance = context;
    BitBuffer* rx_buf = event.data.buffer;

    PicopassListenerCommand picopass_cmd = PicopassListenerCommandSilent;
    if(event.type == NfcEventTypeRxEnd) {
        for(size_t i = 0; i < COUNT_OF(picopass_listener_cmd_handlers); i++) {
            if(bit_buffer_get_size(rx_buf) != picopass_listener_cmd_handlers[i].cmd_len_bits) {
                continue;
            }
            if(bit_buffer_get_byte(rx_buf, 0) !=
               picopass_listener_cmd_handlers[i].start_byte_cmd) {
                continue;
            }
            picopass_cmd = picopass_listener_cmd_handlers[i].handler(instance, rx_buf);
            break;
        }
        if(picopass_cmd == PicopassListenerCommandSendSoF) {
            nfc_iso15693_listener_tx_sof(instance->nfc);
        }
    }

    return command;
}

void picopass_listener_start(
    PicopassListener* instance,
    PicopassListenerCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;

    nfc_start(instance->nfc, picopass_listener_start_callback, instance);
}

void picopass_listener_stop(PicopassListener* instance) {
    furi_assert(instance);

    nfc_stop(instance->nfc);
}

const PicopassData* picopass_listener_get_data(PicopassListener* instance) {
    furi_assert(instance);

    return instance->data;
}
