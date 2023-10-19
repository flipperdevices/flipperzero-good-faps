#include "picopass_listener_i.h"

#include <furi/furi.h>

#define PICOPASS_LISTENER_HAS_MASK(x, b) ((x & b) == b)

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

static void picopass_listener_reset(PicopassListener* instance) {
    instance->state = PicopassListenerStateIdle;
}

PicopassListenerCommand
    picopass_listener_actall_handler(PicopassListener* instance, BitBuffer* buf) {
    UNUSED(buf);

    if(instance->state != PicopassListenerStateHalt) {
        instance->state = PicopassListenerStateActive;
    }
    // nfc_set_fdt_listen_fc(instance->nfc, 1000);

    return PicopassListenerCommandSendSoF;
}

PicopassListenerCommand picopass_listener_act_handler(PicopassListener* instance, BitBuffer* buf) {
    UNUSED(buf);

    PicopassListenerCommand command = PicopassListenerCommandSendSoF;

    if(instance->state != PicopassListenerStateActive) {
        command = PicopassListenerCommandSilent;
    }

    return command;
}

PicopassListenerCommand
    picopass_listener_halt_handler(PicopassListener* instance, BitBuffer* buf) {
    UNUSED(buf);

    PicopassListenerCommand command = PicopassListenerCommandSendSoF;

    // Technically we should go to StateHalt, but since we can't detect the field dropping we drop to idle instead
    instance->state = PicopassListenerStateIdle;

    return command;
}

PicopassListenerCommand
    picopass_listener_identify_handler(PicopassListener* instance, BitBuffer* buf) {
    UNUSED(buf);

    PicopassListenerCommand command = PicopassListenerCommandSilent;

    do {
        if(instance->state != PicopassListenerStateActive) break;
        picopass_listener_write_anticoll_csn(instance, instance->tx_buffer);
        PicopassError error = picopass_listener_send_frame(instance, instance->tx_buffer);
        if(error != PicopassErrorNone) {
            FURI_LOG_D(TAG, "Error sending CSN: %d", error);
            break;
        }

        command = PicopassListenerCommandProcessed;
    } while(false);

    return command;
}

PicopassListenerCommand
    picopass_listener_select_handler(PicopassListener* instance, BitBuffer* buf) {
    PicopassListenerCommand command = PicopassListenerCommandSilent;

    do {
        if((instance->state == PicopassListenerStateHalt) ||
           (instance->state == PicopassListenerStateIdle)) {
            bit_buffer_copy_bytes(
                instance->tmp_buffer,
                instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data,
                sizeof(PicopassBlock));
        } else {
            picopass_listener_write_anticoll_csn(instance, instance->tmp_buffer);
        }
        const uint8_t* listener_uid = bit_buffer_get_data(instance->tmp_buffer);
        const uint8_t* received_data = bit_buffer_get_data(buf);

        if(memcmp(listener_uid, &received_data[1], PICOPASS_BLOCK_LEN) != 0) {
            if(instance->state == PicopassListenerStateActive) {
                instance->state = PicopassListenerStateIdle;
            } else if(instance->state == PicopassListenerStateSelected) {
                // Technically we should go to StateHalt, but since we can't detect the field dropping we drop to idle instead
                instance->state = PicopassListenerStateIdle;
            }
            break;
        }

        instance->state = PicopassListenerStateSelected;
        bit_buffer_copy_bytes(
            instance->tx_buffer,
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data,
            sizeof(PicopassBlock));

        PicopassError error = picopass_listener_send_frame(instance, instance->tx_buffer);
        if(error != PicopassErrorNone) {
            FURI_LOG_D(TAG, "Error sending select response: %d", error);
            break;
        }

        command = PicopassListenerCommandProcessed;
    } while(false);

    return command;
}

PicopassListenerCommand
    picopass_listener_read_handler(PicopassListener* instance, BitBuffer* buf) {
    PicopassListenerCommand command = PicopassListenerCommandSilent;

    do {
        uint8_t block_num = bit_buffer_get_byte(buf, 1);
        if(block_num > PICOPASS_MAX_APP_LIMIT) break;

        bit_buffer_reset(instance->tx_buffer);
        if((block_num == PICOPASS_SECURE_KD_BLOCK_INDEX) ||
           (block_num == PICOPASS_SECURE_KC_BLOCK_INDEX)) {
            for(size_t i = 0; i < PICOPASS_BLOCK_LEN; i++) {
                bit_buffer_append_byte(instance->tx_buffer, 0xff);
            }
        } else {
            bit_buffer_copy_bytes(
                instance->tx_buffer, instance->data->AA1[block_num].data, sizeof(PicopassBlock));
        }
        PicopassError error = picopass_listener_send_frame(instance, instance->tx_buffer);
        if(error != PicopassErrorNone) {
            FURI_LOG_D(TAG, "Failed to tx read block response: %d", error);
            break;
        }

        command = PicopassListenerCommandProcessed;
    } while(false);

    return command;
}

static PicopassListenerCommand
    picopass_listener_readcheck(PicopassListener* instance, BitBuffer* buf, uint8_t key_block_num) {
    PicopassListenerCommand command = PicopassListenerCommandSilent;

    do {
        if(instance->state != PicopassListenerStateSelected) break;
        uint8_t block_num = bit_buffer_get_byte(buf, 1);
        if(block_num != PICOPASS_SECURE_EPURSE_BLOCK_INDEX) break;

        // loclass mode doesn't do any card side crypto, just logs the readers crypto, so no-op in this mode
        // we can also no-op if the key block is the same, CHECK re-inits if it failed already
        if((instance->key_block_num != key_block_num) &&
           (instance->mode != PicopassListenerModeLoclass)) {
            instance->key_block_num = key_block_num;
            picopass_listener_init_cipher_state(instance);
        }

        // DATA(8)
        bit_buffer_copy_bytes(
            instance->tx_buffer, instance->data->AA1[block_num].data, sizeof(PicopassBlock));
        NfcError error = nfc_listener_tx(instance->nfc, instance->tx_buffer);
        if(error != NfcErrorNone) {
            FURI_LOG_D(TAG, "Failed to tx read check response: %d", error);
            break;
        }

        command = PicopassListenerCommandProcessed;
    } while(false);

    return command;
}

PicopassListenerCommand
    picopass_listener_readcheck_kd_handler(PicopassListener* instance, BitBuffer* buf) {
    return picopass_listener_readcheck(instance, buf, PICOPASS_SECURE_KD_BLOCK_INDEX);
}

PicopassListenerCommand
    picopass_listener_readcheck_kc_handler(PicopassListener* instance, BitBuffer* buf) {
    return picopass_listener_readcheck(instance, buf, PICOPASS_SECURE_KC_BLOCK_INDEX);
}

PicopassListenerCommand
    picopass_listener_update_handler(PicopassListener* instance, BitBuffer* buf) {
    PicopassListenerCommand command = PicopassListenerCommandSilent;

    do {
        if(instance->mode == PicopassListenerModeLoclass) break;
        if(instance->state != PicopassListenerStateSelected) break;

        PicopassBlock config_block = instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX];
        bool pers_mode = PICOPASS_LISTENER_HAS_MASK(config_block.data[7], PICOPASS_FUSE_PERS);

        const uint8_t* rx_data = bit_buffer_get_data(buf);
        uint8_t block_num = rx_data[1];
        if(block_num == PICOPASS_CSN_BLOCK_INDEX) break; // CSN is always read only
        if(!pers_mode && PICOPASS_LISTENER_HAS_MASK(config_block.data[3], 0x80))
            break; // Chip is in RO mode, no updated possible (even ePurse)
        if(!pers_mode && (block_num == PICOPASS_SECURE_AIA_BLOCK_INDEX))
            break; // AIA can only be set in personalisation mode
        if(!pers_mode &&
           ((block_num == PICOPASS_SECURE_KD_BLOCK_INDEX ||
             block_num == PICOPASS_SECURE_KC_BLOCK_INDEX) &&
            (!PICOPASS_LISTENER_HAS_MASK(config_block.data[7], PICOPASS_FUSE_CRYPT10))))
            break;

        if(block_num >= 6 && block_num <= 12) {
            // bit0 is block6, up to bit6 being block12
            if(!PICOPASS_LISTENER_HAS_MASK(config_block.data[3], (1 << (block_num - 6)))) {
                // Block is marked as read-only, deny writing
                break;
            }
        }
        // TODO: Check CRC/SIGN depending on if in secure mode
        // Check correct key
        // -> Kd only allows decrementing e-Purse
        // -> per-app controlled by key access config
        // bool keyAccess = PICOPASS_LISTENER_HAS_MASK(config_block.data[5], 0x01);
        // -> must auth with that key to change it

        PicopassBlock new_block = {};
        switch(block_num) {
        case PICOPASS_CONFIG_BLOCK_INDEX:
            new_block.data[0] = config_block.data[0]; // Applications Limit
            new_block.data[1] = config_block.data[1] & rx_data[3]; // OTP
            new_block.data[2] = config_block.data[2] & rx_data[4]; // OTP
            new_block.data[3] = config_block.data[3] & rx_data[5]; // Block Write Lock
            new_block.data[4] = config_block.data[4]; // Chip Config
            new_block.data[5] = config_block.data[5]; // Memory Config
            new_block.data[6] = rx_data[8]; // EAS
            new_block.data[7] = config_block.data[7]; // Fuses

            // Some parts allow w (but not e) if in persMode
            if(pers_mode) {
                new_block.data[0] &= rx_data[2]; // Applications Limit
                new_block.data[4] &= rx_data[6]; // Chip Config
                new_block.data[5] &= rx_data[7]; // Memory Config
                new_block.data[7] &= rx_data[9]; // Fuses
            } else {
                // Fuses allows setting Crypt1/0 from 1 to 0 only during application mode
                new_block.data[7] &= rx_data[9] | ~PICOPASS_FUSE_CRYPT10;
            }
            break;

        case PICOPASS_SECURE_EPURSE_BLOCK_INDEX:
            // ePurse updates swap first and second half of the block each update
            memcpy(&new_block.data[4], &rx_data[2], 4);
            memcpy(&new_block.data[0], &rx_data[6], 4);
            break;

        case PICOPASS_SECURE_KD_BLOCK_INDEX:
            // fallthrough
        case PICOPASS_SECURE_KC_BLOCK_INDEX:
            if(!pers_mode) {
                new_block = instance->data->AA1[block_num];
                for(size_t i = 0; i < sizeof(PicopassBlock); i++) {
                    new_block.data[i] ^= rx_data[i + 2];
                }
                break;
            }
            // Use default case when in personalisation mode
            // fallthrough
        default:
            memcpy(new_block.data, &rx_data[2], sizeof(PicopassBlock));
            break;
        }

        instance->data->AA1[block_num] = new_block;
        if((block_num == instance->key_block_num) ||
           (block_num == PICOPASS_SECURE_EPURSE_BLOCK_INDEX)) {
            picopass_listener_init_cipher_state(instance);
        }

        bit_buffer_reset(instance->tx_buffer);
        if((block_num == PICOPASS_SECURE_KD_BLOCK_INDEX) ||
           (block_num == PICOPASS_SECURE_KC_BLOCK_INDEX)) {
            // Key updates always return FF's
            for(size_t i = 0; i < PICOPASS_BLOCK_LEN; i++) {
                bit_buffer_append_byte(instance->tx_buffer, 0xff);
            }
        } else {
            bit_buffer_copy_bytes(
                instance->tx_buffer, instance->data->AA1[block_num].data, sizeof(PicopassBlock));
        }

        PicopassError error = picopass_listener_send_frame(instance, instance->tx_buffer);
        if(error != PicopassErrorNone) {
            FURI_LOG_D(TAG, "Failed to tx update response: %d", error);
            break;
        }

        command = PicopassListenerCommandProcessed;
    } while(false);

    return command;
}

static const PicopassListenerCmd picopass_listener_cmd_handlers[] = {
    {
        .start_byte_cmd = PICOPASS_CMD_ACTALL,
        .cmd_len_bits = 8,
        .handler = picopass_listener_actall_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_ACT,
        .cmd_len_bits = 8,
        .handler = picopass_listener_act_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_HALT,
        .cmd_len_bits = 8,
        .handler = picopass_listener_halt_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_READ_OR_IDENTIFY,
        .cmd_len_bits = 8,
        .handler = picopass_listener_identify_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_SELECT,
        .cmd_len_bits = 8 * 9,
        .handler = picopass_listener_select_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_READ_OR_IDENTIFY,
        .cmd_len_bits = 8 * 4,
        .handler = picopass_listener_read_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_READCHECK_KD,
        .cmd_len_bits = 8 * 2,
        .handler = picopass_listener_readcheck_kd_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_READCHECK_KC,
        .cmd_len_bits = 8 * 2,
        .handler = picopass_listener_readcheck_kc_handler,
    },
    {
        .start_byte_cmd = PICOPASS_CMD_UPDATE,
        .cmd_len_bits = 8 * 14,
        .handler = picopass_listener_update_handler,
    },
};

PicopassListener* picopass_listener_alloc(Nfc* nfc, const PicopassData* data) {
    furi_assert(nfc);
    furi_assert(data);

    PicopassListener* instance = malloc(sizeof(PicopassListener));
    instance->nfc = nfc;
    instance->data = picopass_protocol_alloc();
    picopass_protocol_copy(instance->data, data);

    instance->tx_buffer = bit_buffer_alloc(PICOPASS_LISTENER_BUFFER_SIZE_MAX);
    instance->tmp_buffer = bit_buffer_alloc(PICOPASS_LISTENER_BUFFER_SIZE_MAX);

    instance->event.data = &instance->event_data;

    nfc_set_fdt_listen_fc(instance->nfc, PICOPASS_FDT_LISTEN_FC);
    nfc_config(instance->nfc, NfcModeListener, NfcTechIso15693);

    return instance;
}

void picopass_listener_free(PicopassListener* instance) {
    furi_assert(instance);

    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->tmp_buffer);
    picopass_protocol_free(instance->data);
    free(instance);
}

void picopass_listener_set_loclass_mode(PicopassListener* instance) {
    furi_assert(instance);

    instance->mode = PicopassListenerModeLoclass;
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

    picopass_listener_reset(instance);
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
