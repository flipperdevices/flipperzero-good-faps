#include "gen2_poller_i.h"
#include <nfc/helpers/nfc_data_generator.h>

#include <furi/furi.h>

#define GEN2_POLLER_THREAD_FLAG_DETECTED (1U << 0)

#define TAG "GEN2"

typedef NfcCommand (*Gen2PollerStateHandler)(Gen2Poller* instance);

typedef struct {
    Nfc* nfc;
    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;
    FuriThreadId thread_id;
    bool detected;
} Gen2PollerDetectContext;

Gen2Poller* gen2_poller_alloc(Nfc* nfc) {
    Gen2Poller* instance = malloc(sizeof(Gen2Poller));
    instance->poller = nfc_poller_alloc(nfc, NfcProtocolIso14443_3a);
    instance->data = mf_classic_alloc();
    instance->crypto = crypto1_alloc();
    instance->tx_plain_buffer = bit_buffer_alloc(GEN2_POLLER_MAX_BUFFER_SIZE);
    instance->tx_encrypted_buffer = bit_buffer_alloc(GEN2_POLLER_MAX_BUFFER_SIZE);
    instance->rx_plain_buffer = bit_buffer_alloc(GEN2_POLLER_MAX_BUFFER_SIZE);
    instance->rx_encrypted_buffer = bit_buffer_alloc(GEN2_POLLER_MAX_BUFFER_SIZE);
    instance->card_state = Gen2CardStateLost;

    instance->gen2_event.data = &instance->gen2_event_data;

    instance->mode_ctx.write_ctx.mfc_data_source = malloc(sizeof(MfClassicData));
    instance->mode_ctx.write_ctx.mfc_data_target = malloc(sizeof(MfClassicData));

    instance->mode_ctx.write_ctx.need_halt_before_write = true;

    return instance;
}

void gen2_poller_free(Gen2Poller* instance) {
    furi_assert(instance);
    furi_assert(instance->data);
    furi_assert(instance->crypto);
    furi_assert(instance->tx_plain_buffer);
    furi_assert(instance->rx_plain_buffer);
    furi_assert(instance->tx_encrypted_buffer);
    furi_assert(instance->rx_encrypted_buffer);

    nfc_poller_free(instance->poller);
    mf_classic_free(instance->data);
    crypto1_free(instance->crypto);
    bit_buffer_free(instance->tx_plain_buffer);
    bit_buffer_free(instance->rx_plain_buffer);
    bit_buffer_free(instance->tx_encrypted_buffer);
    bit_buffer_free(instance->rx_encrypted_buffer);

    free(instance->mode_ctx.write_ctx.mfc_data_source);
    free(instance->mode_ctx.write_ctx.mfc_data_target);

    free(instance);
}

// static void gen2_poller_reset(Gen2Poller* instance) {
//     instance->current_block = 0;
//     nfc_data_generator_fill_data(NfcDataGeneratorTypeMfClassic1k_4b, instance->mfc_device);
// }

NfcCommand gen2_poller_idle_handler(Gen2Poller* instance) {
    furi_assert(instance);

    NfcCommand command = NfcCommandContinue;

    instance->mode_ctx.write_ctx.current_block = 0;
    instance->gen2_event.type = Gen2PollerEventTypeDetected;
    command = instance->callback(instance->gen2_event, instance->context);
    instance->state = Gen2PollerStateRequestMode;

    return command;
}

NfcCommand gen2_poller_request_mode_handler(Gen2Poller* instance) {
    furi_assert(instance);

    NfcCommand command = NfcCommandContinue;

    instance->gen2_event.type = Gen2PollerEventTypeRequestMode;
    command = instance->callback(instance->gen2_event, instance->context);
    if(instance->gen2_event_data.poller_mode.mode == Gen2PollerModeWipe) {
        instance->state = Gen2PollerStateWipe;
    } else {
        instance->state = Gen2PollerStateWriteSourceDataRequest;
    }

    return command;
}

NfcCommand gen2_poller_wipe_handler(Gen2Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    Gen2PollerError error = Gen2PollerErrorNone;
    UNUSED(error);
    UNUSED(instance);
    // TODO: Implement wipe

    return command;
}

NfcCommand gen2_poller_write_source_data_request_handler(Gen2Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->gen2_event.type = Gen2PollerEventTypeRequestDataToWrite;
    command = instance->callback(instance->gen2_event, instance->context);
    memcpy(
        instance->mode_ctx.write_ctx.mfc_data_source,
        instance->gen2_event_data.data_to_write.mfc_data,
        sizeof(MfClassicData));
    instance->state = Gen2PollerStateWriteTargetDataRequest;

    return command;
}

NfcCommand gen2_poller_write_target_data_request_handler(Gen2Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->gen2_event.type = Gen2PollerEventTypeRequestTargetData;
    command = instance->callback(instance->gen2_event, instance->context);
    memcpy(
        instance->mode_ctx.write_ctx.mfc_data_target,
        instance->gen2_event_data.target_data.mfc_data,
        sizeof(MfClassicData));
    instance->state = Gen2PollerStateWrite;

    return command;
}

Gen2PollerError gen2_poller_write_block_handler(
    Gen2Poller* instance,
    uint8_t block_num,
    MfClassicBlock* block) {
    furi_assert(instance);

    Gen2PollerError error = Gen2PollerErrorNone;
    Gen2PollerWriteContext* write_ctx = &instance->mode_ctx.write_ctx;
    MfClassicKey auth_key = write_ctx->auth_key;

    do {
        // Compare the target and source data
        if(memcmp(block->data, write_ctx->mfc_data_target->block[block_num].data, 16) == 0) {
            FURI_LOG_D(TAG, "Block %d is the same, skipping", block_num);
            break;
        }
        // Reauth if necessary
        if(write_ctx->need_halt_before_write) {
            FURI_LOG_D(TAG, "Auth before writing block %d", write_ctx->current_block);
            error = gen2_poller_auth(
                instance, write_ctx->current_block, &auth_key, write_ctx->write_key, NULL);
            if(error != Gen2PollerErrorNone) {
                FURI_LOG_D(
                    TAG, "Failed to auth to block %d for writing", write_ctx->current_block);
                instance->state = Gen2PollerStateFail;
                break;
            }
        }

        // Write the block
        error = gen2_poller_write_block(instance, write_ctx->current_block, block);
        if(error != Gen2PollerErrorNone) {
            FURI_LOG_D(TAG, "Failed to write block %d", write_ctx->current_block);
            instance->state = Gen2PollerStateFail;
            break;
        }
    } while(false);
    FURI_LOG_D(TAG, "Block %d finished, halting", write_ctx->current_block);
    gen2_poller_halt(instance);
    FURI_LOG_D(TAG, "Halt finished");
    return error;
}

NfcCommand gen2_poller_write_handler(Gen2Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    Gen2PollerError error = Gen2PollerErrorNone;
    Gen2PollerWriteContext* write_ctx = &instance->mode_ctx.write_ctx;
    uint8_t block_num = write_ctx->current_block;

    do {
        // Check whether the block is present in the source data
        if(!mf_classic_is_block_read(write_ctx->mfc_data_source, block_num)) {
            // FURI_LOG_E(TAG, "Block %d not present in source data", block_num);
            break;
        }
        // Check whether the ACs for that block are known in target data
        if(!mf_classic_is_block_read(
               write_ctx->mfc_data_target,
               mf_classic_get_sector_trailer_num_by_block(block_num))) {
            FURI_LOG_E(TAG, "Sector trailer for block %d not present in target data", block_num);
            break;
        }
        // Check whether ACs need to be reset and whether they can be reset
        if(!gen2_poller_can_write_block(write_ctx->mfc_data_target, block_num)) {
            if(!gen2_can_reset_access_conditions(write_ctx->mfc_data_target, block_num)) {
                FURI_LOG_E(TAG, "Block %d cannot be written", block_num);
                break;
            } else {
                FURI_LOG_D(TAG, "Resetting ACs for block %d", block_num);
                // Generate a block with old keys and default ACs (0xFF, 0x07, 0x80)
                MfClassicBlock block;
                memset(&block, 0, sizeof(block));
                memcpy(block.data, write_ctx->mfc_data_target->block[block_num].data, 16);
                memcpy(block.data + 6, "\xFF\x07\x80", 3);
                error = gen2_poller_write_block_handler(instance, block_num, &block);
                if(error != Gen2PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to reset ACs for block %d", block_num);
                    break;
                } else {
                    FURI_LOG_D(TAG, "ACs for block %d reset", block_num);
                    memcpy(write_ctx->mfc_data_target->block[block_num].data, block.data, 16);
                }
            }
        }
        // Figure out which key to use for writing
        // TODO: MfClassicKeyTypeNone?
        write_ctx->write_key =
            gen2_poller_get_key_type_to_write(write_ctx->mfc_data_target, block_num);

        // Get the key to use for writing from the target data
        MfClassicSectorTrailer* sec_tr = mf_classic_get_sector_trailer_by_sector(
            write_ctx->mfc_data_target, mf_classic_get_sector_by_block(block_num));
        if(write_ctx->write_key == MfClassicKeyTypeA) {
            FURI_LOG_D(TAG, "Using key A for block %d", block_num);
            FURI_LOG_D(
                TAG,
                "Key A: %02X %02X %02X %02X %02X %02X",
                sec_tr->key_a.data[0],
                sec_tr->key_a.data[1],
                sec_tr->key_a.data[2],
                sec_tr->key_a.data[3],
                sec_tr->key_a.data[4],
                sec_tr->key_a.data[5]);
            write_ctx->auth_key = sec_tr->key_a;
        } else {
            FURI_LOG_D(TAG, "Using key B for block %d", block_num);
            FURI_LOG_D(
                TAG,
                "Key B: %02X %02X %02X %02X %02X %02X",
                sec_tr->key_b.data[0],
                sec_tr->key_b.data[1],
                sec_tr->key_b.data[2],
                sec_tr->key_b.data[3],
                sec_tr->key_b.data[4],
                sec_tr->key_b.data[5]);
            write_ctx->auth_key = sec_tr->key_b;
        }

        // Write the block
        error = gen2_poller_write_block_handler(
            instance, block_num, &write_ctx->mfc_data_source->block[block_num]);
        if(error != Gen2PollerErrorNone) {
            FURI_LOG_E(TAG, "Couldn't to write block %d", block_num);
            break;
        }
    } while(false);
    write_ctx->current_block++;

    if(error != Gen2PollerErrorNone) {
        FURI_LOG_D(TAG, "Error occurred, stopping: %d", error);
        instance->state = Gen2PollerStateFail;
    } else if(
        write_ctx->current_block ==
        mf_classic_get_total_block_num(write_ctx->mfc_data_source->type)) {
        instance->state = Gen2PollerStateSuccess;
    }

    return command;
}

NfcCommand gen2_poller_success_handler(Gen2Poller* instance) {
    furi_assert(instance);

    NfcCommand command = NfcCommandContinue;

    instance->gen2_event.type = Gen2PollerEventTypeSuccess;
    command = instance->callback(instance->gen2_event, instance->context);
    instance->state = Gen2PollerStateIdle;

    return command;
}

NfcCommand gen2_poller_fail_handler(Gen2Poller* instance) {
    furi_assert(instance);

    NfcCommand command = NfcCommandContinue;

    instance->gen2_event.type = Gen2PollerEventTypeFail;
    command = instance->callback(instance->gen2_event, instance->context);
    instance->state = Gen2PollerStateIdle;

    return command;
}

static const Gen2PollerStateHandler gen2_poller_state_handlers[Gen2PollerStateNum] = {
    [Gen2PollerStateIdle] = gen2_poller_idle_handler,
    [Gen2PollerStateRequestMode] = gen2_poller_request_mode_handler,
    [Gen2PollerStateWipe] = gen2_poller_wipe_handler,
    [Gen2PollerStateWriteSourceDataRequest] = gen2_poller_write_source_data_request_handler,
    [Gen2PollerStateWriteTargetDataRequest] = gen2_poller_write_target_data_request_handler,
    [Gen2PollerStateWrite] = gen2_poller_write_handler,
    [Gen2PollerStateSuccess] = gen2_poller_success_handler,
    [Gen2PollerStateFail] = gen2_poller_fail_handler,
};

NfcCommand gen2_poller_callback(NfcGenericEvent event, void* context) {
    furi_assert(context);
    furi_assert(event.protocol == NfcProtocolIso14443_3a);
    furi_assert(event.event_data);
    furi_assert(event.instance);

    NfcCommand command = NfcCommandContinue;
    Gen2Poller* instance = context;
    instance->iso3_poller = event.instance;
    Iso14443_3aPollerEvent* iso3_event = event.event_data;

    if(iso3_event->type == Iso14443_3aPollerEventTypeReady) {
        command = gen2_poller_state_handlers[instance->state](instance);
    }

    return command;
}

void gen2_poller_start(Gen2Poller* instance, Gen2PollerCallback callback, void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;

    nfc_poller_start(instance->poller, gen2_poller_callback, instance);
    return;
}

void gen2_poller_stop(Gen2Poller* instance) {
    furi_assert(instance);

    FURI_LOG_D(TAG, "Stopping Gen2 poller");
    nfc_poller_stop(instance->poller);
    return;
}

bool gen2_poller_can_write_everything(NfcDevice* device) {
    furi_assert(device);

    bool can_write = true;
    const MfClassicData* mfc_data = nfc_device_get_data(device, NfcProtocolMfClassic);

    if(mfc_data) {
        uint16_t total_block_num = mf_classic_get_total_block_num(mfc_data->type);
        for(uint16_t i = 0; i < total_block_num; i++) {
            if(mf_classic_is_sector_trailer(i)) {
                if(!gen2_poller_can_write_sector_trailer(mfc_data, i)) {
                    can_write = false;
                    break;
                }
            } else {
                if(!gen2_poller_can_write_data_block(mfc_data, i)) {
                    can_write = false;
                    break;
                }
            }
        }
    }

    return can_write;
}