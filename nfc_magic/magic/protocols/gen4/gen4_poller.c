#include "bit_buffer.h"
#include "core/check.h"
#include "gen4_poller_i.h"
#include "magic/protocols/gen4/gen4.h"
#include "magic/protocols/gen4/gen4_poller.h"
#include <nfc/protocols/iso14443_3a/iso14443_3a.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <nfc/nfc_poller.h>
#include <bit_lib.h>
#include <string.h>

#define GEN4_POLLER_THREAD_FLAG_DETECTED (1U << 0)
#define GEN4_POLLER_DEFAULT_CONFIG_SIZE  (28)

typedef NfcCommand (*Gen4PollerStateHandler)(Gen4Poller* instance);

typedef struct {
    NfcPoller* poller;
    Gen4Password password;
    Gen4 gen4_data;
    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;
    FuriThreadId thread_id;
    Gen4PollerError error;
} Gen4PollerDetectContext;

static const Gen4Config gen4_poller_default_config = {
    .data_raw = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x09, 0x78,
                 0x00, 0x91, 0x02, 0xDA, 0xBC, 0x19, 0x10, 0x10, 0x11, 0x12,
                 0x13, 0x14, 0x15, 0x16, 0x04, 0x00, 0x08, 0x00}};

static const uint8_t gen4_poller_default_block_0[GEN4_POLLER_BLOCK_SIZE] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t gen4_poller_default_empty_block[GEN4_POLLER_BLOCK_SIZE] = {0};

static const uint8_t gen4_poller_default_sector_trailer_block[GEN4_POLLER_BLOCK_SIZE] =
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x69, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static bool gen4_poller_is_sector_trailer(uint8_t block_num) {
    uint8_t sec_tr_block_num = 0;

    if(block_num < 128) {
        sec_tr_block_num = block_num | 0x03;
    } else {
        sec_tr_block_num = block_num | 0x0f;
    }

    return block_num == sec_tr_block_num;
}

Gen4Poller* gen4_poller_alloc(Nfc* nfc) {
    furi_assert(nfc);

    Gen4Poller* instance = malloc(sizeof(Gen4Poller));
    instance->poller = nfc_poller_alloc(nfc, NfcProtocolIso14443_3a);

    instance->gen4_event.data = &instance->gen4_event_data;

    instance->tx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);
    instance->rx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);

    instance->gen4_data = gen4_alloc();
    instance->cmd_step = 0;

    return instance;
}

void gen4_poller_free(Gen4Poller* instance) {
    furi_assert(instance);

    nfc_poller_free(instance->poller);

    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);

    gen4_free(instance->gen4_data);

    free(instance);
}

void gen4_poller_set_password(Gen4Poller* instance, Gen4Password password) {
    furi_assert(instance);

    instance->password = password;
}

NfcCommand gen4_poller_detect_callback(NfcGenericEvent event, void* context) {
    furi_assert(context);
    furi_assert(event.protocol == NfcProtocolIso14443_3a);
    furi_assert(event.instance);
    furi_assert(event.event_data);

    NfcCommand command = NfcCommandStop;
    Gen4PollerDetectContext* gen4_poller_detect_ctx = context;
    Iso14443_3aPoller* iso3_poller = event.instance;
    Iso14443_3aPollerEvent* iso3_event = event.event_data;
    gen4_poller_detect_ctx->error = Gen4PollerErrorTimeout;

    if(iso3_event->type == Iso14443_3aPollerEventTypeReady) {
        do {
            // check config
            bit_buffer_append_byte(gen4_poller_detect_ctx->tx_buffer, GEN4_CMD_PREFIX);
            bit_buffer_append_bytes(
                gen4_poller_detect_ctx->tx_buffer,
                gen4_poller_detect_ctx->password.bytes,
                GEN4_PASSWORD_LEN);
            bit_buffer_append_byte(gen4_poller_detect_ctx->tx_buffer, GEN4_CMD_GET_CFG);

            Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
                iso3_poller,
                gen4_poller_detect_ctx->tx_buffer,
                gen4_poller_detect_ctx->rx_buffer,
                GEN4_POLLER_MAX_FWT);

            if(error != Iso14443_3aErrorNone) {
                gen4_poller_detect_ctx->error = Gen4PollerErrorProtocol;
                break;
            }
            size_t rx_bytes = bit_buffer_get_size_bytes(gen4_poller_detect_ctx->rx_buffer);
            if(rx_bytes != GEN4_CONFIG_SIZE) {
                gen4_poller_detect_ctx->error = Gen4PollerErrorProtocol;
                break;
            }

            memcpy(
                gen4_poller_detect_ctx->gen4_data.config.data_raw,
                bit_buffer_get_data(gen4_poller_detect_ctx->rx_buffer),
                GEN4_CONFIG_SIZE);

            // check revision
            bit_buffer_reset(gen4_poller_detect_ctx->tx_buffer);
            bit_buffer_reset(gen4_poller_detect_ctx->rx_buffer);

            bit_buffer_append_byte(gen4_poller_detect_ctx->tx_buffer, GEN4_CMD_PREFIX);
            bit_buffer_append_bytes(
                gen4_poller_detect_ctx->tx_buffer,
                gen4_poller_detect_ctx->password.bytes,
                GEN4_PASSWORD_LEN);
            bit_buffer_append_byte(gen4_poller_detect_ctx->tx_buffer, GEN4_CMD_GET_REVISION);

            error = iso14443_3a_poller_send_standard_frame(
                iso3_poller,
                gen4_poller_detect_ctx->tx_buffer,
                gen4_poller_detect_ctx->rx_buffer,
                GEN4_POLLER_MAX_FWT);

            if(error != Iso14443_3aErrorNone) {
                gen4_poller_detect_ctx->error = Gen4PollerErrorProtocol;
                break;
            }
            rx_bytes = bit_buffer_get_size_bytes(gen4_poller_detect_ctx->rx_buffer);
            if(rx_bytes != GEN4_REVISION_SIZE) {
                gen4_poller_detect_ctx->error = Gen4PollerErrorProtocol;
                break;
            }

            memcpy(
                gen4_poller_detect_ctx->gen4_data.revision.data,
                bit_buffer_get_data(gen4_poller_detect_ctx->rx_buffer),
                GEN4_REVISION_SIZE);

            gen4_poller_detect_ctx->error = Gen4PollerErrorNone;
        } while(false);
    } else if(iso3_event->type == Iso14443_3aPollerEventTypeError) {
        gen4_poller_detect_ctx->error = Gen4PollerErrorTimeout;
    }
    furi_thread_flags_set(gen4_poller_detect_ctx->thread_id, GEN4_POLLER_THREAD_FLAG_DETECTED);

    return command;
}

Gen4PollerError gen4_poller_detect(Nfc* nfc, Gen4Password password, Gen4* gen4_data) {
    furi_assert(nfc);

    Gen4PollerDetectContext gen4_poller_detect_ctx = {};
    gen4_poller_detect_ctx.poller = nfc_poller_alloc(nfc, NfcProtocolIso14443_3a);
    gen4_poller_detect_ctx.password = password;
    gen4_poller_detect_ctx.tx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);
    gen4_poller_detect_ctx.rx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);
    gen4_poller_detect_ctx.thread_id = furi_thread_get_current_id();
    gen4_poller_detect_ctx.error = Gen4PollerErrorNone;

    nfc_poller_start(
        gen4_poller_detect_ctx.poller, gen4_poller_detect_callback, &gen4_poller_detect_ctx);
    uint32_t flags =
        furi_thread_flags_wait(GEN4_POLLER_THREAD_FLAG_DETECTED, FuriFlagWaitAny, FuriWaitForever);
    if(flags & GEN4_POLLER_THREAD_FLAG_DETECTED) {
        furi_thread_flags_clear(GEN4_POLLER_THREAD_FLAG_DETECTED);
    }
    nfc_poller_stop(gen4_poller_detect_ctx.poller);

    nfc_poller_free(gen4_poller_detect_ctx.poller);
    bit_buffer_free(gen4_poller_detect_ctx.tx_buffer);
    bit_buffer_free(gen4_poller_detect_ctx.rx_buffer);

    if(gen4_poller_detect_ctx.error == Gen4PollerErrorNone) {
        gen4_copy(gen4_data, &gen4_poller_detect_ctx.gen4_data);
    }

    return gen4_poller_detect_ctx.error;
}

NfcCommand gen4_poller_idle_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->current_block = 0;

    instance->gen4_event.type = Gen4PollerEventTypeCardDetected;
    command = instance->callback(instance->gen4_event, instance->context);
    instance->state = Gen4PollerStateRequestMode;

    return command;
}

NfcCommand gen4_poller_request_mode_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->gen4_event.type = Gen4PollerEventTypeRequestMode;
    command = instance->callback(instance->gen4_event, instance->context);
    if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWipe) {
        instance->state = Gen4PollerStateWipe;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWrite) {
        instance->state = Gen4PollerStateRequestWriteData;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetPassword) {
        instance->state = Gen4PollerStateChangePassword;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeGetInfo) {
        instance->state = Gen4PollerStateGetInfo;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetDefaultCfg) {
        instance->state = Gen4PollerStateSetDefaultConfig;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetShadowMode) {
        instance->state = Gen4PollerStateSetShadowMode;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetDirectWriteBlock0Mode) {
        instance->state = Gen4PollerStateSetDirectWriteBlock0;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWriteUID) {
        instance->state = Gen4PollerStateWriteUID;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetATQASAK) {
        instance->state = Gen4PollerStateSetATQASAK;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetATS) {
        instance->state = Gen4PollerStateSetATS;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetULProtocol) {
        instance->state = Gen4PollerStateSetULProtocol;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetULMode) {
        instance->state = Gen4PollerStateSetULMode;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetMaxRWBlock) {
        instance->state = Gen4PollerStateSetMaxRWBlock;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeSetType) {
        instance->state = Gen4PollerStateSetType;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWriteNTAGPwd) {
        instance->state = Gen4PollerStateWriteNTAGPwd;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWritePack) {
        instance->state = Gen4PollerStateWritePack;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWriteOTP) {
        instance->state = Gen4PollerStateWriteOTP;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWriteVersion) {
        instance->state = Gen4PollerStateWriteVersion;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeWriteSignature) {
        instance->state = Gen4PollerStateWriteSignature;
    } else if(instance->gen4_event_data.request_mode.mode == Gen4PollerModeFullWipe) {
        instance->state = Gen4PollerStateFullWipe;
    } else {
        instance->state = Gen4PollerStateFail;
    }

    return command;
}

NfcCommand gen4_poller_wipe_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = Gen4PollerErrorNone;
        if(instance->current_block == 0) {
            error = gen4_poller_set_config(
                instance,
                instance->password,
                &gen4_poller_default_config,
                GEN4_POLLER_DEFAULT_CONFIG_SIZE,
                false);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to set default config: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
            gen4_password_reset(&instance->password);
            error = gen4_poller_write_block(
                instance, instance->password, instance->current_block, gen4_poller_default_block_0);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to write 0 block: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        } else if(instance->current_block < GEN4_POLLER_BLOCKS_TOTAL) {
            const uint8_t* block = gen4_poller_is_sector_trailer(instance->current_block) ?
                                       gen4_poller_default_sector_trailer_block :
                                       gen4_poller_default_empty_block;
            error = gen4_poller_write_block(
                instance, instance->password, instance->current_block, block);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to write %d block: %d", instance->current_block, error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        } else {
            instance->state = Gen4PollerStateSuccess;
            break;
        }
        instance->current_block++;
    } while(false);

    return command;
}

NfcCommand gen4_poller_request_write_data_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->gen4_event.type = Gen4PollerEventTypeRequestDataToWrite;
    command = instance->callback(instance->gen4_event, instance->context);
    instance->protocol = instance->gen4_event_data.request_data.protocol;
    instance->data = instance->gen4_event_data.request_data.data;

    if((instance->protocol == NfcProtocolMfClassic) ||
       (instance->protocol == NfcProtocolMfUltralight)) {
        instance->state = Gen4PollerStateWrite;
    } else {
        FURI_LOG_E(TAG, "Unsupported protocol");
        instance->state = Gen4PollerStateFail;
    }

    return command;
}

static NfcCommand gen4_poller_write_mf_classic(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        const MfClassicData* mfc_data = instance->data;
        const Iso14443_3aData* iso3_data = mfc_data->iso14443_3a_data;
        if(instance->current_block == 0) {
            instance->config.data_parsed.protocol = Gen4ProtocolMfClassic;
            instance->total_blocks = mf_classic_get_total_block_num(mfc_data->type);

            if(iso3_data->uid_len == 4) {
                instance->config.data_parsed.uid_len_code = Gen4UIDLengthSingle;
            } else if(iso3_data->uid_len == 7) {
                instance->config.data_parsed.uid_len_code = Gen4UIDLengthDouble;
            } else {
                FURI_LOG_E(TAG, "Unsupported UID len: %d", iso3_data->uid_len);
                instance->state = Gen4PollerStateFail;
                break;
            }

            instance->config.data_parsed.gtu_mode = Gen4ShadowModeDisabled;
            instance->config.data_parsed.atqa[0] = iso3_data->atqa[0];
            instance->config.data_parsed.atqa[1] = iso3_data->atqa[1];
            instance->config.data_parsed.sak = iso3_data->sak;
            instance->config.data_parsed.mfu_mode = Gen4UltralightModeUL_EV1;
            instance->config.data_parsed.total_blocks = instance->total_blocks - 1;
            instance->config.data_parsed.direct_write_mode = Gen4DirectWriteBlock0ModeDisabled;

            Gen4PollerError error = gen4_poller_set_config(
                instance, instance->password, &instance->config, GEN4_CONFIG_SIZE, false);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to write config: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        }
        if(instance->current_block < instance->total_blocks) {
            FURI_LOG_D(TAG, "Writing block %d", instance->current_block);
            Gen4PollerError error = gen4_poller_write_block(
                instance,
                instance->password,
                instance->current_block,
                mfc_data->block[instance->current_block].data);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to write %d block: %d", instance->current_block, error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        } else {
            instance->state = Gen4PollerStateSuccess;
            break;
        }
        instance->current_block++;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_write_mf_ultralight(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        const MfUltralightData* mfu_data = instance->data;
        const Iso14443_3aData* iso3_data = mfu_data->iso14443_3a_data;
        if(instance->current_block == 0) {
            instance->total_blocks = 64;
            instance->config.data_parsed.protocol = Gen4ProtocolMfUltralight;
            switch(mfu_data->type) {
            case MfUltralightTypeNTAG203:
                FURI_LOG_D(TAG, "NTAG203 type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeNTAG;
                instance->total_blocks = 42;
                break;
            case MfUltralightTypeNTAG213:
                FURI_LOG_D(TAG, "NTAG213 type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeNTAG;
                instance->total_blocks = 45;
                break;
            case MfUltralightTypeNTAG215:
                FURI_LOG_D(TAG, "NTAG215 type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeNTAG;
                instance->total_blocks = 135;
                break;
            case MfUltralightTypeNTAG216:
                FURI_LOG_D(TAG, "NTAG216 type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeNTAG;
                instance->total_blocks = 231;
                break;
            case MfUltralightTypeNTAGI2C1K:
            case MfUltralightTypeNTAGI2C2K:
            case MfUltralightTypeNTAGI2CPlus1K:
            case MfUltralightTypeNTAGI2CPlus2K:
                FURI_LOG_D(TAG, "NTAG type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeNTAG;
                instance->total_blocks = 128;
                break;

            case MfUltralightTypeOrigin:
                FURI_LOG_D(TAG, "Ultralight type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeUL;
                break;

            case MfUltralightTypeMfulC:
                FURI_LOG_D(TAG, "MfulC type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeUL_C;
                break;

            case MfUltralightTypeUL11:
            case MfUltralightTypeUL21:
            default:
                FURI_LOG_D(TAG, "EV1 type");
                instance->config.data_parsed.mfu_mode = Gen4UltralightModeUL_EV1;
                break;
            }

            if(iso3_data->uid_len == 4) {
                instance->config.data_parsed.uid_len_code = Gen4UIDLengthSingle;
            } else if(iso3_data->uid_len == 7) {
                instance->config.data_parsed.uid_len_code = Gen4UIDLengthDouble;
            } else {
                FURI_LOG_E(TAG, "Unsupported UID len: %d", iso3_data->uid_len);
                instance->state = Gen4PollerStateFail;
                break;
            }

            instance->config.data_parsed.gtu_mode = Gen4ShadowModeHighSpeedDisabled;
            instance->config.data_parsed.atqa[0] = iso3_data->atqa[0];
            instance->config.data_parsed.atqa[1] = iso3_data->atqa[1];
            instance->config.data_parsed.sak = iso3_data->sak;
            instance->config.data_parsed.total_blocks = instance->total_blocks - 1;
            instance->config.data_parsed.direct_write_mode = Gen4DirectWriteBlock0ModeDisabled;

            Gen4PollerError error = gen4_poller_set_config(
                instance, instance->password, &instance->config, GEN4_CONFIG_SIZE, false);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to write config: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        }

        if(instance->current_block < mfu_data->pages_read) {
            FURI_LOG_D(
                TAG, "Writing page %zu / %zu", instance->current_block, mfu_data->pages_read);
            Gen4PollerError error = gen4_poller_write_block(
                instance,
                instance->password,
                instance->current_block,
                mfu_data->page[instance->current_block].data);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_D(TAG, "Failed to write %d page: %d", instance->current_block, error);
                instance->state = Gen4PollerStateFail;
                break;
            }
            instance->current_block++;
        } else {
            uint8_t block[GEN4_POLLER_BLOCK_SIZE] = {};
            bool write_success = true;

            if(mf_ultralight_support_feature(
                   mf_ultralight_get_feature_support_set(mfu_data->type),
                   MfUltralightFeatureSupportReadSignature)) {
                FURI_LOG_D(TAG, "Writing Signature");
                for(size_t i = 0; i < 8; i++) {
                    memcpy(block, &mfu_data->signature.data[i * 4], 4); //-V1086
                    Gen4PollerError error =
                        gen4_poller_write_block(instance, instance->password, 0xF2 + i, block);
                    if(error != Gen4PollerErrorNone) {
                        write_success = false;
                        break;
                    }
                }
                if(!write_success) {
                    FURI_LOG_E(TAG, "Failed to write Signature");
                    instance->state = Gen4PollerStateFail;
                    break;
                }
            } else {
                FURI_LOG_D(TAG, "Signature is not supported, skipping");
            }

            if(mf_ultralight_support_feature(
                   mf_ultralight_get_feature_support_set(mfu_data->type),
                   MfUltralightFeatureSupportReadVersion)) {
                FURI_LOG_D(TAG, "Writing Version part 1");
                block[0] = mfu_data->version.header;
                block[1] = mfu_data->version.vendor_id;
                block[2] = mfu_data->version.prod_type;
                block[3] = mfu_data->version.prod_subtype;
                Gen4PollerError error =
                    gen4_poller_write_block(instance, instance->password, 0xFA, block);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to write 1st part Version");
                    instance->state = Gen4PollerStateFail;
                    break;
                }

                FURI_LOG_D(TAG, "Writing Version part 2");
                block[0] = mfu_data->version.prod_ver_major;
                block[1] = mfu_data->version.prod_ver_minor;
                block[2] = mfu_data->version.storage_size;
                block[3] = mfu_data->version.protocol_type;
                error = gen4_poller_write_block(instance, instance->password, 0xFB, block);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to write 2nd part Version");
                    instance->state = Gen4PollerStateFail;
                    break;
                }
            } else {
                FURI_LOG_D(TAG, "Version is not supported, skipping");
            }

            if(mf_ultralight_support_feature(
                   mf_ultralight_get_feature_support_set(mfu_data->type),
                   MfUltralightFeatureSupportPasswordAuth)) {
                FURI_LOG_D(TAG, "Writing Password");
                MfUltralightConfigPages* config_pages = NULL;
                if(mf_ultralight_get_config_page(mfu_data, &config_pages)) {
                    block[0] = config_pages->password.data[0];
                    block[1] = config_pages->password.data[1];
                    block[2] = config_pages->password.data[2];
                    block[3] = config_pages->password.data[3];
                    Gen4PollerError error =
                        gen4_poller_write_block(instance, instance->password, 0xE5, block);
                    if(error != Gen4PollerErrorNone) {
                        FURI_LOG_E(TAG, "Failed to write Password to sector E5");
                        instance->state = Gen4PollerStateFail;
                        break;
                    }
                    error = gen4_poller_write_block(instance, instance->password, 0xF0, block);
                    if(error != Gen4PollerErrorNone) {
                        FURI_LOG_E(TAG, "Failed to write Password to sector F0");
                        instance->state = Gen4PollerStateFail;
                        break;
                    }

                    FURI_LOG_D(TAG, "Writing PACK");
                    block[0] = config_pages->pack.data[0];
                    block[1] = config_pages->pack.data[1];
                    block[2] = 0x00;
                    block[3] = 0x00;
                    error = gen4_poller_write_block(instance, instance->password, 0xE6, block);
                    if(error != Gen4PollerErrorNone) {
                        FURI_LOG_E(TAG, "Failed to write PACK to sector E6");
                        instance->state = Gen4PollerStateFail;
                        break;
                    }
                    error = gen4_poller_write_block(instance, instance->password, 0xF1, block);
                    if(error != Gen4PollerErrorNone) {
                        FURI_LOG_E(TAG, "Failed to write PACK to sector F1");
                        instance->state = Gen4PollerStateFail;
                        break;
                    }
                }
            } else {
                FURI_LOG_D(TAG, "Password is not supported, skipping");
            }

            instance->state = Gen4PollerStateSuccess;
        }
    } while(false);

    return command;
}

NfcCommand gen4_poller_write_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    memcpy(
        instance->config.data_raw,
        gen4_poller_default_config.data_raw,
        GEN4_POLLER_DEFAULT_CONFIG_SIZE);

    memcpy(
        instance->config.data_parsed.password.bytes, instance->password.bytes, GEN4_PASSWORD_LEN);
    memset(&instance->config.data_raw[7], 0, 17);
    if(instance->protocol == NfcProtocolMfClassic) {
        command = gen4_poller_write_mf_classic(instance);
    } else if(instance->protocol == NfcProtocolMfUltralight) {
        command = gen4_poller_write_mf_ultralight(instance);
    } else {
        furi_crash("Unsupported protocol to write");
    }

    return command;
}

NfcCommand gen4_poller_change_password_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        instance->gen4_event.type = Gen4PollerEventTypeRequestNewPassword;
        command = instance->callback(instance->gen4_event, instance->context);
        if(command != NfcCommandContinue) break;

        Gen4Password new_password = instance->gen4_event_data.request_password.password;
        Gen4PollerError error =
            gen4_poller_change_password(instance, instance->password, new_password);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to change password: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        instance->password = new_password;
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_set_default_cfg_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = gen4_poller_set_config(
            instance,
            instance->password,
            &gen4_poller_default_config,
            GEN4_POLLER_DEFAULT_CONFIG_SIZE,
            false);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set default config: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_get_current_cfg_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4Config config = {};

        Gen4PollerError error = gen4_poller_get_config(instance, instance->password, &config);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to get current config: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        // Copy config data to event data buffer
        memcpy(instance->gen4_data->config.data_raw, config.data_raw, sizeof(config));

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_get_revision_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4Revision revision = {};
        Gen4PollerError error = gen4_poller_get_revision(instance, instance->password, &revision);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to get revision: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        // Copy revision data to event data buffer
        memcpy(instance->gen4_data->revision.data, revision.data, sizeof(revision));

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_get_info_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4 gen4_data;

        Gen4PollerError error =
            gen4_poller_get_revision(instance, instance->password, &gen4_data.revision);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to get revision: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        error = gen4_poller_get_config(instance, instance->password, &gen4_data.config);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to get current config: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        // Copy config&&revision data to event data buffer
        gen4_copy(instance->gen4_data, &gen4_data);

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_set_shadow_mode_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error =
            gen4_poller_set_shadow_mode(instance, instance->password, instance->shadow_mode);

        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set shadow mode: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_set_direct_write_block_0_mode_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = gen4_poller_set_direct_write_block_0_mode(
            instance, instance->password, instance->direct_write_block_0_mode);

        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set direct write to block 0 mode: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

NfcCommand gen4_poller_success_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->gen4_event.type = Gen4PollerEventTypeSuccess;
    command = instance->callback(instance->gen4_event, instance->context);
    if(command != NfcCommandStop) {
        furi_delay_ms(100);
    }

    return command;
}

NfcCommand gen4_poller_fail_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->gen4_event.type = Gen4PollerEventTypeFail;
    command = instance->callback(instance->gen4_event, instance->context);
    if(command != NfcCommandStop) {
        furi_delay_ms(100);
    }

    return command;
}

static NfcCommand gen4_poller_write_uid_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        if(instance->cmd_step == 0) {
            Gen4PollerError error =
                gen4_poller_get_config(instance, instance->password, &instance->config);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to read config for UID write: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
            instance->cmd_step = 1;
        }

        if(instance->config.data_parsed.protocol == Gen4ProtocolMfClassic) {
            uint8_t block0[GEN4_POLLER_BLOCK_SIZE] = {};
            gen4_calc_mfc_block0(instance->uid, instance->uid_len, block0);
            Gen4PollerError error =
                gen4_poller_write_block(instance, instance->password, 0, block0);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to write MFC block0: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        } else {
            uint8_t bcc0, bcc1;
            gen4_calc_mfu_bcc(instance->uid, &bcc0, &bcc1);
            uint8_t block_data[4];

            block_data[0] = instance->uid[0];
            block_data[1] = instance->uid[1];
            block_data[2] = instance->uid[2];
            block_data[3] = bcc0;
            Gen4PollerError error =
                gen4_poller_mfu_write_block(instance, 0, block_data);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to write MFU block0: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }

            block_data[0] = instance->uid[3];
            block_data[1] = instance->uid[4];
            block_data[2] = instance->uid[5];
            block_data[3] = instance->uid[6];
            error = gen4_poller_mfu_write_block(instance, 1, block_data);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to write MFU block1: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }

            block_data[0] = bcc1;
            block_data[1] = 0x48;
            block_data[2] = 0x00;
            block_data[3] = 0x00;
            error = gen4_poller_mfu_write_block(instance, 2, block_data);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to write MFU block2: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
        }

        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_set_atqa_sak_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = gen4_poller_set_atqa_sak(
            instance, instance->password, instance->atqa0, instance->atqa1, instance->sak);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set ATQA/SAK: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_set_ats_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = gen4_poller_set_ats(
            instance, instance->password, instance->ats, instance->ats_len);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set ATS: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_set_ul_protocol_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error =
            gen4_poller_set_ul_protocol(instance, instance->password, instance->ul_protocol);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set UL protocol: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_set_ul_mode_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error =
            gen4_poller_set_ul_mode(instance, instance->password, instance->ul_mode);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set UL mode: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_set_max_rw_block_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error =
            gen4_poller_set_max_rw_block(instance, instance->password, instance->max_rw_block);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to set max R/W block: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_set_type_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        if(instance->cmd_step == 0) {
            if(instance->tag_type <= 9) {
                uint8_t config_data[] = {
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x09, 0x78,
                    0x00, 0x91, 0x02, 0xDA, 0xBC, 0x19, 0x10, 0x10, 0x11, 0x12,
                    0x13, 0x14, 0x15, 0x16, 0x04, 0x00, 0x08, 0x00};
                uint8_t config_size = 28;

                switch(instance->tag_type) {
                case 1:
                    FURI_LOG_D(TAG, "Set type: Mifare Mini S20 4-byte");
                    config_data[3] = 0x02;
                    config_data[24] = 0x09;
                    break;
                case 2:
                    FURI_LOG_D(TAG, "Set type: Mifare Mini S20 7-byte");
                    config_data[0] = 0x01;
                    config_data[3] = 0x02;
                    config_data[24] = 0x09;
                    break;
                case 3:
                    FURI_LOG_D(TAG, "Set type: Mifare Mini S20 10-byte");
                    config_data[0] = 0x02;
                    config_data[3] = 0x02;
                    config_data[24] = 0x09;
                    break;
                case 4:
                    FURI_LOG_D(TAG, "Set type: Mifare 1k S50 4-byte");
                    config_data[3] = 0x02;
                    config_data[24] = 0x08;
                    break;
                case 5:
                    FURI_LOG_D(TAG, "Set type: Mifare 1k S50 7-byte");
                    config_data[0] = 0x01;
                    config_data[3] = 0x02;
                    config_data[24] = 0x08;
                    break;
                case 6:
                    FURI_LOG_D(TAG, "Set type: Mifare 1k S50 10-byte");
                    config_data[0] = 0x02;
                    config_data[3] = 0x02;
                    config_data[24] = 0x08;
                    break;
                case 7:
                    FURI_LOG_D(TAG, "Set type: Mifare 4k S70 4-byte");
                    config_data[3] = 0x02;
                    config_data[24] = 0x18;
                    break;
                case 8:
                    FURI_LOG_D(TAG, "Set type: Mifare 4k S70 7-byte");
                    config_data[0] = 0x01;
                    config_data[3] = 0x02;
                    config_data[24] = 0x18;
                    break;
                case 9:
                    FURI_LOG_D(TAG, "Set type: Mifare 4k S70 10-byte");
                    config_data[0] = 0x02;
                    config_data[3] = 0x02;
                    config_data[24] = 0x18;
                    break;
                default:
                    FURI_LOG_E(TAG, "Unsupported MFC tag type: %d", instance->tag_type);
                    instance->state = Gen4PollerStateFail;
                    break;
                }

                if(instance->state == Gen4PollerStateFail) break;

                Gen4Config set_config = {};
                memcpy(set_config.data_raw, config_data, config_size);
                memcpy(
                    set_config.data_parsed.password.bytes,
                    instance->password.bytes,
                    GEN4_PASSWORD_LEN);

                Gen4PollerError error = gen4_poller_set_config(
                    instance, instance->password, &set_config, config_size, false);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to set MFC type config: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
            } else {
                uint8_t config_data[32] = {0};
                config_data[0] = 0x01;
                config_data[2] = 0x01;
                config_data[13] = 0x03;
                config_data[17] = 0x09;
                config_data[18] = 0x78;
                config_data[20] = 0x91;
                config_data[21] = 0x02;
                config_data[22] = 0xDA;
                config_data[23] = 0xBC;
                config_data[24] = 0x19;
                config_data[25] = 0x10;
                config_data[26] = 0x10;
                config_data[27] = 0x11;
                config_data[28] = 0x12;
                config_data[29] = 0x13;
                config_data[30] = 0x14;
                config_data[31] = 0x15;

                switch(instance->tag_type) {
                case 10:
                    FURI_LOG_D(TAG, "Set type: UL");
                    config_data[30] = 0x03;
                    break;
                case 11:
                    FURI_LOG_D(TAG, "Set type: UL-C");
                    config_data[30] = 0x02;
                    break;
                case 12:
                    FURI_LOG_D(TAG, "Set type: UL EV1 48");
                    config_data[30] = 0x00;
                    break;
                case 13:
                    FURI_LOG_D(TAG, "Set type: UL EV1 128");
                    config_data[30] = 0x00;
                    break;
                default:
                    FURI_LOG_D(TAG, "Set type: NTAG");
                    config_data[30] = 0x01;
                    break;
                }
                config_data[31] = 0x16;
                if(instance->tag_type == 12 || instance->tag_type == 13) {
                    config_data[31] = 0x16;
                }

                uint8_t config_size = 32;
                Gen4Config set_config = {};
                memcpy(set_config.data_raw, config_data, config_size);
                memcpy(
                    set_config.data_parsed.password.bytes,
                    instance->password.bytes,
                    GEN4_PASSWORD_LEN);

                Gen4PollerError error = gen4_poller_set_config(
                    instance, instance->password, &set_config, config_size, false);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to set UL type config: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }

                instance->cmd_step = 1;
            }
        }

        if(instance->tag_type >= 10 && instance->tag_type <= 25) {
            if(instance->cmd_step >= 1 && instance->cmd_step <= 3) {
                uint8_t cc_blocks[12];
                uint8_t has_cc = 0;

                switch(instance->tag_type) {
                case 12:
                case 13:
                case 10:
                case 11:
                case 14:
                    has_cc = 0;
                    break;
                case 15:
                    cc_blocks[0] = 0xE1;
                    cc_blocks[1] = 0x10;
                    cc_blocks[2] = 0x06;
                    cc_blocks[3] = 0x00;
                    cc_blocks[4] = 0x03;
                    cc_blocks[5] = 0x00;
                    cc_blocks[6] = 0xFE;
                    cc_blocks[7] = 0x00;
                    cc_blocks[8] = 0x00;
                    cc_blocks[9] = 0x00;
                    cc_blocks[10] = 0x00;
                    cc_blocks[11] = 0x00;
                    has_cc = 1;
                    break;
                case 16:
                    cc_blocks[0] = 0xE1;
                    cc_blocks[1] = 0x10;
                    cc_blocks[2] = 0x10;
                    cc_blocks[3] = 0x00;
                    cc_blocks[4] = 0x01;
                    cc_blocks[5] = 0x03;
                    cc_blocks[6] = 0x90;
                    cc_blocks[7] = 0x0A;
                    cc_blocks[8] = 0x34;
                    cc_blocks[9] = 0x03;
                    cc_blocks[10] = 0x00;
                    cc_blocks[11] = 0xFE;
                    has_cc = 1;
                    break;
                case 17:
                case 24:
                    cc_blocks[0] = 0xE1;
                    cc_blocks[1] = 0x10;
                    cc_blocks[2] = 0x12;
                    cc_blocks[3] = 0x00;
                    cc_blocks[4] = 0x01;
                    cc_blocks[5] = 0x03;
                    cc_blocks[6] = 0xA0;
                    cc_blocks[7] = 0x0C;
                    cc_blocks[8] = 0x34;
                    cc_blocks[9] = 0x03;
                    cc_blocks[10] = 0x00;
                    cc_blocks[11] = 0xFE;
                    has_cc = 1;
                    break;
                case 18:
                case 19:
                case 20:
                case 21:
                case 22:
                case 23:
                case 25:
                    cc_blocks[0] = 0xE1;
                    cc_blocks[1] = 0x10;
                    cc_blocks[2] = 0x6D;
                    cc_blocks[3] = 0x00;
                    cc_blocks[4] = 0x03;
                    cc_blocks[5] = 0x00;
                    cc_blocks[6] = 0xFE;
                    cc_blocks[7] = 0x00;
                    cc_blocks[8] = 0x00;
                    cc_blocks[9] = 0x00;
                    cc_blocks[10] = 0x00;
                    cc_blocks[11] = 0x00;
                    has_cc = 1;
                    break;
                default:
                    has_cc = 0;
                    break;
                }

                if(has_cc) {
                    uint8_t blk_data[4];
                    memcpy(blk_data, &cc_blocks[(instance->cmd_step - 1) * 4], 4);
                    Gen4PollerError error = gen4_poller_mfu_write_block(
                        instance, 3 + (instance->cmd_step - 1), blk_data);
                    if(error != Gen4PollerErrorNone) {
                        FURI_LOG_D(
                            TAG,
                            "Failed to write CC block %d: %d",
                            instance->cmd_step - 1,
                            error);
                    }
                }

                instance->cmd_step++;
            }

            if(instance->cmd_step >= 4) {
                instance->state = Gen4PollerStateSuccess;
            }
        } else if(instance->tag_type >= 1 && instance->tag_type <= 9) {
            instance->state = Gen4PollerStateSuccess;
        } else {
            FURI_LOG_E(TAG, "Invalid tag type: %d", instance->tag_type);
            instance->state = Gen4PollerStateFail;
        }
    } while(false);

    return command;
}

static NfcCommand gen4_poller_write_ntag_pwd_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = gen4_poller_mfu_write_block(instance, 0xE5, instance->ntag_pwd);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to write NTAG PWD to E5: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        error = gen4_poller_mfu_write_block(instance, 0xF0, instance->ntag_pwd);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to write NTAG PWD to F0: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_write_pack_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        uint8_t block_data[4] = {instance->pack[0], instance->pack[1], 0x00, 0x00};
        Gen4PollerError error = gen4_poller_mfu_write_block(instance, 0xE6, block_data);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to write PACK to E6: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        error = gen4_poller_mfu_write_block(instance, 0xF1, block_data);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to write PACK to F1: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_write_otp_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        Gen4PollerError error = gen4_poller_mfu_write_block(instance, 3, instance->otp);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(TAG, "Failed to write OTP: %d", error);
            instance->state = Gen4PollerStateFail;
            break;
        }
        instance->state = Gen4PollerStateSuccess;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_write_version_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        if(instance->cmd_step == 0) {
            Gen4PollerError error =
                gen4_poller_set_max_rw_block(instance, instance->password, 0xFB);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to set max R/W block for version write: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
            instance->cmd_step = 1;
        }

        uint8_t block_data[4];
        memcpy(block_data, &instance->version[(instance->cmd_step - 1) * 4], 4);
        Gen4PollerError error =
            gen4_poller_mfu_write_block(instance, 0xFA + (instance->cmd_step - 1), block_data);
        if(error != Gen4PollerErrorNone) {
            FURI_LOG_E(
                TAG, "Failed to write version block %d: %d", instance->cmd_step - 1, error);
            instance->state = Gen4PollerStateFail;
            break;
        }

        if(instance->cmd_step >= 2) {
            instance->state = Gen4PollerStateSuccess;
            break;
        }
        instance->cmd_step++;
    } while(false);

    return command;
}

static NfcCommand gen4_poller_write_signature_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        if(instance->cmd_step == 0) {
            Gen4PollerError error =
                gen4_poller_get_config(instance, instance->password, &instance->config);
            if(error != Gen4PollerErrorNone) {
                FURI_LOG_E(TAG, "Failed to read config for signature write: %d", error);
                instance->state = Gen4PollerStateFail;
                break;
            }
            instance->cmd_step = 1;
        }

        if(instance->config.data_parsed.protocol == Gen4ProtocolMfClassic) {
            if(instance->cmd_step == 1) {
                Gen4PollerError error =
                    gen4_poller_set_max_rw_block(instance, instance->password, 0x48);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to set max R/W for MFC signature: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                instance->cmd_step = 2;
            }

            uint8_t block_idx = instance->cmd_step - 2;
            if(block_idx < 2) {
                uint8_t block[GEN4_POLLER_BLOCK_SIZE] = {};
                memcpy(block, &instance->signature[block_idx * 16], 16);
                Gen4PollerError error =
                    gen4_poller_write_block(instance, instance->password, 0x45 + block_idx, block);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to write MFC sig block %d: %d", block_idx, error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                instance->cmd_step++;
            }

            if(instance->cmd_step >= 4) {
                uint8_t final_block[16] = {
                    0x5C, 0x8F, 0xF9, 0x99, 0x0D, 0xA2, 0x70, 0xF0,
                    0xF8, 0x69, 0x4B, 0x79, 0x1B, 0xEA, 0x7B, 0xCC};
                Gen4PollerError error = gen4_poller_write_block(
                    instance, instance->password, 0x47, final_block);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to write MFC sig final block: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                instance->state = Gen4PollerStateSuccess;
            }
        } else {
            if(instance->cmd_step == 1) {
                Gen4PollerError error =
                    gen4_poller_set_max_rw_block(instance, instance->password, 0xFB);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to set max R/W for UL signature: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                instance->cmd_step = 2;
            }

            uint8_t blk_idx = instance->cmd_step - 2;
            if(blk_idx < 8) {
                uint8_t block_data[4];
                memcpy(block_data, &instance->signature[blk_idx * 4], 4);
                Gen4PollerError error =
                    gen4_poller_mfu_write_block(instance, 0xF2 + blk_idx, block_data);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to write UL sig block %d: %d", blk_idx, error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                instance->cmd_step++;
            }

            if(instance->cmd_step >= 10) {
                instance->state = Gen4PollerStateSuccess;
            }
        }
    } while(false);

    return command;
}

static NfcCommand gen4_poller_full_wipe_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        if(instance->wipe_type == 0) {
            if(instance->cmd_step == 0) {
                Gen4PollerError error = gen4_poller_set_config(
                    instance,
                    instance->password,
                    &gen4_poller_default_config,
                    GEN4_POLLER_DEFAULT_CONFIG_SIZE,
                    false);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_D(TAG, "Failed to set default config for wipe: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                gen4_password_reset(&instance->password);
                Gen4PollerError error2 = gen4_poller_write_block(
                    instance, instance->password, 0, gen4_poller_default_block_0);
                if(error2 != Gen4PollerErrorNone) {
                    FURI_LOG_D(TAG, "Failed to write block 0 for wipe: %d", error2);
                    instance->state = Gen4PollerStateFail;
                    break;
                }
                instance->cmd_step = 1;
                instance->current_block = 1;
            }

            while(instance->current_block < GEN4_POLLER_BLOCKS_TOTAL) {
                const uint8_t* block = gen4_poller_is_sector_trailer(instance->current_block) ?
                                           gen4_poller_default_sector_trailer_block :
                                           gen4_poller_default_empty_block;
                Gen4PollerError error = gen4_poller_write_block(
                    instance, instance->password, instance->current_block, block);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_D(
                        TAG, "Failed to write block %d for wipe: %d", instance->current_block, error);
                }
                instance->current_block++;
            }

            if(instance->current_block >= GEN4_POLLER_BLOCKS_TOTAL) {
                instance->state = Gen4PollerStateSuccess;
            }
        } else if(instance->wipe_type == 1) {
            if(instance->cmd_step == 0) {
                uint8_t ul_config[32] = {0};
                ul_config[0] = 0x01;
                ul_config[2] = 0x01;
                ul_config[29] = 0x14;
                ul_config[30] = 0x01;
                ul_config[31] = 0x16;
                Gen4Config config = {};
                memcpy(config.data_raw, ul_config, 32);
                memcpy(config.data_parsed.password.bytes, instance->password.bytes, GEN4_PASSWORD_LEN);

                Gen4PollerError error = gen4_poller_set_config(
                    instance, instance->password, &config, sizeof(ul_config), false);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to set UL config for wipe: %d", error);
                    instance->state = Gen4PollerStateFail;
                    break;
                }

                Gen4PollerError error2 =
                    gen4_poller_set_max_rw_block(instance, instance->password, 0xFB);
                if(error2 != Gen4PollerErrorNone) {
                    FURI_LOG_E(TAG, "Failed to set max R/W for UL wipe: %d", error2);
                }

                instance->cmd_step = 1;
                instance->current_block = 3;
            }

            while(instance->current_block <= 0xFB) {
                uint8_t block_data[4] = {0};
                uint16_t b = instance->current_block;
                if(b == 0x29 || b == 0x83 || b == 0xE3) {
                    block_data[2] = 0xFF;
                } else if(b == 0x2A || b == 0x84 || b == 0xE4) {
                    block_data[1] = 0x05;
                }
                Gen4PollerError error =
                    gen4_poller_mfu_write_block(instance, instance->current_block, block_data);
                if(error != Gen4PollerErrorNone) {
                    FURI_LOG_D(
                        TAG,
                        "Failed to wipe UL block %d: %d",
                        instance->current_block,
                        error);
                }
                instance->current_block++;
            }

            if(instance->current_block > 0xFB) {
                instance->state = Gen4PollerStateSuccess;
            }
        } else {
            FURI_LOG_E(TAG, "Invalid wipe type: %d", instance->wipe_type);
            instance->state = Gen4PollerStateFail;
        }
    } while(false);

    return command;
}

static const Gen4PollerStateHandler gen4_poller_state_handlers[Gen4PollerStateNum] = {
    [Gen4PollerStateIdle] = gen4_poller_idle_handler,
    [Gen4PollerStateRequestMode] = gen4_poller_request_mode_handler,
    [Gen4PollerStateRequestWriteData] = gen4_poller_request_write_data_handler,
    [Gen4PollerStateWrite] = gen4_poller_write_handler,
    [Gen4PollerStateWipe] = gen4_poller_wipe_handler,
    [Gen4PollerStateChangePassword] = gen4_poller_change_password_handler,
    [Gen4PollerStateGetInfo] = gen4_poller_get_info_handler,
    [Gen4PollerStateSetDefaultConfig] = gen4_poller_set_default_cfg_handler,
    [Gen4PollerStateSetShadowMode] = gen4_poller_set_shadow_mode_handler,
    [Gen4PollerStateSetDirectWriteBlock0] = gen4_poller_set_direct_write_block_0_mode_handler,
    [Gen4PollerStateWriteUID] = gen4_poller_write_uid_handler,
    [Gen4PollerStateSetATQASAK] = gen4_poller_set_atqa_sak_handler,
    [Gen4PollerStateSetATS] = gen4_poller_set_ats_handler,
    [Gen4PollerStateSetULProtocol] = gen4_poller_set_ul_protocol_handler,
    [Gen4PollerStateSetULMode] = gen4_poller_set_ul_mode_handler,
    [Gen4PollerStateSetMaxRWBlock] = gen4_poller_set_max_rw_block_handler,
    [Gen4PollerStateSetType] = gen4_poller_set_type_handler,
    [Gen4PollerStateWriteNTAGPwd] = gen4_poller_write_ntag_pwd_handler,
    [Gen4PollerStateWritePack] = gen4_poller_write_pack_handler,
    [Gen4PollerStateWriteOTP] = gen4_poller_write_otp_handler,
    [Gen4PollerStateWriteVersion] = gen4_poller_write_version_handler,
    [Gen4PollerStateWriteSignature] = gen4_poller_write_signature_handler,
    [Gen4PollerStateFullWipe] = gen4_poller_full_wipe_handler,
    [Gen4PollerStateSuccess] = gen4_poller_success_handler,
    [Gen4PollerStateFail] = gen4_poller_fail_handler,

};

static NfcCommand gen4_poller_callback(NfcGenericEvent event, void* context) {
    furi_assert(context);
    furi_assert(event.protocol == NfcProtocolIso14443_3a);
    furi_assert(event.event_data);
    furi_assert(event.instance);

    NfcCommand command = NfcCommandContinue;
    Gen4Poller* instance = context;
    instance->iso3_poller = event.instance;
    Iso14443_3aPollerEvent* iso3_event = event.event_data;

    if(iso3_event->type == Iso14443_3aPollerEventTypeReady) {
        command = gen4_poller_state_handlers[instance->state](instance);
    }

    return command;
}

void gen4_poller_start(Gen4Poller* instance, Gen4PollerCallback callback, void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;

    nfc_poller_start(instance->poller, gen4_poller_callback, instance);
}

void gen4_poller_stop(Gen4Poller* instance) {
    furi_assert(instance);

    nfc_poller_stop(instance->poller);
}

const Gen4* gen4_poller_get_gen4_data(const Gen4Poller* instance) {
    furi_assert(instance);

    return instance->gen4_data;
}

void gen4_poller_struct_set_direct_write_block_0_mode(
    Gen4Poller* instance,
    Gen4DirectWriteBlock0Mode mode) {
    furi_assert(instance);

    instance->direct_write_block_0_mode = mode;
}

void gen4_poller_struct_set_shadow_mode(Gen4Poller* instance, Gen4ShadowMode mode) {
    furi_assert(instance);

    instance->shadow_mode = mode;
}

void gen4_poller_struct_set_uid(Gen4Poller* instance, const uint8_t* uid, uint8_t uid_len) {
    furi_assert(instance);
    furi_assert(uid_len <= GEN4_UID_MAX_LEN);

    memcpy(instance->uid, uid, uid_len);
    instance->uid_len = uid_len;
}

void gen4_poller_struct_set_atqa_sak_params(
    Gen4Poller* instance,
    uint8_t atqa0,
    uint8_t atqa1,
    uint8_t sak) {
    furi_assert(instance);

    instance->atqa0 = atqa0;
    instance->atqa1 = atqa1;
    instance->sak = sak;
}

void gen4_poller_struct_set_ats_params(
    Gen4Poller* instance,
    const uint8_t* ats,
    uint8_t ats_len) {
    furi_assert(instance);
    furi_assert(ats_len <= GEN4_ATS_MAX_LEN);

    memcpy(instance->ats, ats, ats_len);
    instance->ats_len = ats_len;
}

void gen4_poller_struct_set_protocol(Gen4Poller* instance, Gen4Protocol protocol) {
    furi_assert(instance);

    instance->ul_protocol = protocol;
}

void gen4_poller_struct_set_ul_mode(Gen4Poller* instance, Gen4UltralightMode mode) {
    furi_assert(instance);

    instance->ul_mode = mode;
}

void gen4_poller_struct_set_max_rw_block(Gen4Poller* instance, uint8_t max_block) {
    furi_assert(instance);

    instance->max_rw_block = max_block;
}

void gen4_poller_struct_set_tag_type(Gen4Poller* instance, uint8_t tag_type) {
    furi_assert(instance);

    instance->tag_type = tag_type;
}

void gen4_poller_struct_set_ntag_pwd(Gen4Poller* instance, const uint8_t* pwd) {
    furi_assert(instance);

    memcpy(instance->ntag_pwd, pwd, 4);
}

void gen4_poller_struct_set_pack(Gen4Poller* instance, const uint8_t* pack) {
    furi_assert(instance);

    memcpy(instance->pack, pack, 2);
}

void gen4_poller_struct_set_otp(Gen4Poller* instance, const uint8_t* otp) {
    furi_assert(instance);

    memcpy(instance->otp, otp, 4);
}

void gen4_poller_struct_set_version(Gen4Poller* instance, const uint8_t* version) {
    furi_assert(instance);

    memcpy(instance->version, version, 8);
}

void gen4_poller_struct_set_signature(Gen4Poller* instance, const uint8_t* signature) {
    furi_assert(instance);

    memcpy(instance->signature, signature, GEN4_SIGNATURE_SIZE);
}

void gen4_poller_struct_set_wipe_type(Gen4Poller* instance, uint8_t wipe_type) {
    furi_assert(instance);

    instance->wipe_type = wipe_type;
}
