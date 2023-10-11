#include "picopass_poller_i.h"

#include "../loclass/optimized_cipher.h"

#include <furi/furi.h>

#define TAG "Picopass"

typedef NfcCommand (*PicopassPollerStateHandler)(PicopassPoller* instance);

static void picopass_poller_reset(PicopassPoller* instance) {
    instance->current_block = 0;
}

static void picopass_poller_prepare_read(PicopassPoller* instance) {
    instance->app_limit = instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0] <
                                  PICOPASS_MAX_APP_LIMIT ?
                              instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0] :
                              PICOPASS_MAX_APP_LIMIT;
    instance->current_block = 2;
}

NfcCommand picopass_poller_detect_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;
    PicopassError error = picopass_poller_actall(instance);

    if(error == PicopassErrorNone) {
        instance->state = PicopassPollerStatePreAuth;
    }

    return command;
}

NfcCommand picopass_poller_pre_auth_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        PicopassError error = picopass_poller_identify(instance, &instance->col_res_serial_num);
        if(error != PicopassErrorNone) {
            instance->state = PicopassPollerStateFail;
            break;
        }

        error =
            picopass_poller_select(instance, &instance->col_res_serial_num, &instance->serial_num);
        if(error != PicopassErrorNone) {
            instance->state = PicopassPollerStateFail;
            break;
        }
        memcpy(
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data,
            instance->serial_num.data,
            sizeof(PicopassSerialNum));
        FURI_LOG_D(
            TAG,
            "csn %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[0],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[1],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[2],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[3],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[4],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[5],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[6],
            instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data[7]);

        PicopassBlock block = {};
        error = picopass_poller_read_block(instance, 1, &block);
        if(error != PicopassErrorNone) {
            instance->state = PicopassPollerStateFail;
            break;
        }
        memcpy(
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data,
            block.data,
            sizeof(PicopassBlock));
        FURI_LOG_D(
            TAG,
            "config %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[1],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[2],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[3],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[4],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[5],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[6],
            instance->data->AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[7]);

        error = picopass_poller_read_block(instance, 5, &block);
        if(error != PicopassErrorNone) {
            instance->state = PicopassPollerStateFail;
            break;
        }
        memcpy(
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data,
            block.data,
            sizeof(PicopassBlock));
        FURI_LOG_D(
            TAG,
            "aia %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[0],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[1],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[2],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[3],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[4],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[5],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[6],
            instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[7]);

        instance->state = PicopassPollerStateCheckSecurity;
    } while(false);

    return command;
}

NfcCommand picopass_poller_check_security(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    // Thank you proxmark!
    PicopassBlock temp_block = {};
    memset(temp_block.data, 0xff, sizeof(PicopassBlock));
    instance->data->pacs.legacy =
        (memcmp(
             instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data,
             temp_block.data,
             sizeof(PicopassBlock)) == 0);

    temp_block.data[3] = 0x00;
    temp_block.data[4] = 0x06;
    instance->data->pacs.se_enabled =
        (memcmp(
             instance->data->AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data,
             temp_block.data,
             sizeof(PicopassBlock)) == 0);

    if(instance->data->pacs.se_enabled) {
        FURI_LOG_D(TAG, "SE enabled");
        instance->state = PicopassPollerStateFail;
    } else {
        instance->state = PicopassPollerStateAuth;
    }

    return command;
}

NfcCommand picopass_poller_auth_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        // Request key
        instance->event.type = PicopassPollerEventTypeRequestKey;
        command = instance->callback(instance->event, instance->context);
        if(command != NfcCommandContinue) break;

        if(!instance->event_data.req_key.is_key_provided) {
            instance->state = PicopassPollerStateFail;
            break;
        }

        PicopassReadCheckResp read_check_resp = {};
        uint8_t* csn = instance->data->AA1[PICOPASS_CSN_BLOCK_INDEX].data;
        uint8_t* div_key = instance->data->AA1[PICOPASS_SECURE_KD_BLOCK_INDEX].data;

        uint8_t ccnr[12] = {};
        PicopassMac mac = {};

        PicopassError error = picopass_poller_read_check(instance, &read_check_resp);
        if(error != PicopassErrorNone) {
            FURI_LOG_E(TAG, "Read check failed: %d", error);
            break;
        }
        memcpy(ccnr, read_check_resp.data, sizeof(PicopassReadCheckResp)); // last 4 bytes left 0

        loclass_iclass_calc_div_key(
            csn,
            instance->event_data.req_key.key,
            div_key,
            instance->event_data.req_key.is_elite_key);
        loclass_opt_doReaderMAC(ccnr, div_key, mac.data);

        PicopassCheckResp check_resp = {};
        error = picopass_poller_check(instance, &mac, &check_resp);
        if(error == PicopassErrorNone) {
            FURI_LOG_I(TAG, "Found key");
            memcpy(instance->data->pacs.key, instance->event_data.req_key.key, PICOPASS_KEY_LEN);
            picopass_poller_prepare_read(instance);
            instance->state = PicopassPollerStateReadBlock;
        }

    } while(false);

    return command;
}

NfcCommand picopass_poller_read_block_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    do {
        if(instance->current_block == instance->app_limit) {
            instance->state = PicopassPollerStateParseCredential;
            break;
        }

        if(instance->current_block == PICOPASS_SECURE_KD_BLOCK_INDEX) {
            // Skip over Kd block which is populated earlier (READ of Kd returns all FF's)
            instance->current_block++;
        }

        PicopassBlock block = {};
        PicopassError error =
            picopass_poller_read_block(instance, instance->current_block, &block);
        if(error != PicopassErrorNone) {
            FURI_LOG_E(TAG, "Failed to read block %d: %d", instance->current_block, error);
            instance->state = PicopassPollerStateFail;
            break;
        }
        FURI_LOG_D(
            TAG,
            "Block %d: %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->current_block,
            block.data[0],
            block.data[1],
            block.data[2],
            block.data[3],
            block.data[4],
            block.data[5],
            block.data[6],
            block.data[7]);
        memcpy(
            instance->data->AA1[instance->current_block].data, block.data, sizeof(PicopassBlock));
        instance->current_block++;
    } while(false);

    return command;
}

NfcCommand picopass_poller_parse_credential_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    picopass_protocol_parse_credential(instance->data);
    instance->state = PicopassPollerStateParseWiegand;
    return command;
}

NfcCommand picopass_poller_parse_wiegand_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    picopass_protocol_parse_wiegand(instance->data);
    instance->state = PicopassPollerStateSuccess;
    return command;
}

NfcCommand picopass_poller_success_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    instance->event.type = PicopassPollerEventTypeSuccess;
    command = instance->callback(instance->event, instance->context);
    furi_delay_ms(100);

    return command;
}

NfcCommand picopass_poller_fail_handler(PicopassPoller* instance) {
    NfcCommand command = NfcCommandReset;

    instance->event.type = PicopassPollerEventTypeFail;
    command = instance->callback(instance->event, instance->context);
    picopass_poller_reset(instance);
    instance->state = PicopassPollerStateDetect;

    return command;
}

static const PicopassPollerStateHandler picopass_poller_state_handler[PicopassPollerStateNum] = {
    [PicopassPollerStateDetect] = picopass_poller_detect_handler,
    [PicopassPollerStatePreAuth] = picopass_poller_pre_auth_handler,
    [PicopassPollerStateCheckSecurity] = picopass_poller_check_security,
    [PicopassPollerStateAuth] = picopass_poller_auth_handler,
    [PicopassPollerStateReadBlock] = picopass_poller_read_block_handler,
    [PicopassPollerStateParseCredential] = picopass_poller_parse_credential_handler,
    [PicopassPollerStateParseWiegand] = picopass_poller_parse_wiegand_handler,
    [PicopassPollerStateSuccess] = picopass_poller_success_handler,
    [PicopassPollerStateFail] = picopass_poller_fail_handler,
};

static NfcCommand picopass_poller_callback(NfcEvent event, void* context) {
    furi_assert(context);

    PicopassPoller* instance = context;
    NfcCommand command = NfcCommandContinue;

    if(event.type == NfcEventTypePollerReady) {
        command = picopass_poller_state_handler[instance->state](instance);
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

PicopassPoller* picopass_poller_alloc(Nfc* nfc) {
    furi_assert(nfc);

    PicopassPoller* instance = malloc(sizeof(PicopassPoller));
    instance->nfc = nfc;
    nfc_config(instance->nfc, NfcModePoller, NfcTechIso15693);
    nfc_set_guard_time_us(instance->nfc, 10000);
    nfc_set_fdt_poll_fc(instance->nfc, 5000);
    nfc_set_fdt_poll_poll_us(instance->nfc, 1000);

    instance->event.data = &instance->event_data;
    instance->data = picopass_protocol_alloc();

    instance->tx_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);
    instance->rx_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);
    instance->tmp_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);

    return instance;
}

void picopass_poller_free(PicopassPoller* instance) {
    furi_assert(instance);

    picopass_protocol_free(instance->data);
    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);
    bit_buffer_free(instance->tmp_buffer);
    free(instance);
}

const PicopassData* picopass_poller_get_data(PicopassPoller* instance) {
    furi_assert(instance);

    return instance->data;
}
