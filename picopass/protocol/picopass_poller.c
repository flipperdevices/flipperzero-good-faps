#include "picopass_poller_i.h"

#include <furi/furi.h>

#define TAG "Picopass"

typedef NfcCommand (*PicopassPollerStateHandler)(PicopassPoller* instance);

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
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data,
            instance->serial_num.data,
            sizeof(PicopassSerialNum));
        FURI_LOG_D(
            TAG,
            "csn %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[0],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[1],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[2],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[3],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[4],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[5],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[6],
            instance->data.AA1[PICOPASS_CSN_BLOCK_INDEX].data[7]);

        PicopassBlock block = {};
        error = picopass_poller_read_block(instance, 1, &block);
        if(error != PicopassErrorNone) {
            instance->state = PicopassPollerStateFail;
            break;
        }
        memcpy(
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data,
            block.data,
            sizeof(PicopassBlock));
        FURI_LOG_D(
            TAG,
            "config %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[0],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[1],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[2],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[3],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[4],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[5],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[6],
            instance->data.AA1[PICOPASS_CONFIG_BLOCK_INDEX].data[7]);

        error = picopass_poller_read_block(instance, 5, &block);
        if(error != PicopassErrorNone) {
            instance->state = PicopassPollerStateFail;
            break;
        }
        memcpy(
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data,
            block.data,
            sizeof(PicopassBlock));
        FURI_LOG_D(
            TAG,
            "aia %02x%02x%02x%02x%02x%02x%02x%02x",
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[0],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[1],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[2],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[3],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[4],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[5],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[6],
            instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data[7]);

        instance->state = PicopassPollerStateCheckSecurity;
    } while(false);

    return command;
}

NfcCommand picopass_poller_check_security(PicopassPoller* instance) {
    NfcCommand command = NfcCommandContinue;

    // Thank you proxmark!
    PicopassBlock temp_block = {};
    memset(temp_block.data, 0xff, sizeof(PicopassBlock));
    instance->data.pacs.legacy =
        (memcmp(
             instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data,
             temp_block.data,
             sizeof(PicopassBlock)) == 0);

    temp_block.data[3] = 0x00;
    temp_block.data[4] = 0x06;
    instance->data.pacs.se_enabled =
        (memcmp(
             instance->data.AA1[PICOPASS_SECURE_AIA_BLOCK_INDEX].data,
             temp_block.data,
             sizeof(PicopassBlock)) == 0);

    if(instance->data.pacs.se_enabled) {
        FURI_LOG_D(TAG, "SE enabled");
        instance->state = PicopassPollerStateFail;
    } else {
        instance->state = PicopassPollerStateSuccess;
    }

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
    instance->state = PicopassPollerStateDetect;

    return command;
}

static const PicopassPollerStateHandler picopass_poller_state_handler[PicopassPollerStateNum] = {
    [PicopassPollerStateDetect] = picopass_poller_detect_handler,
    [PicopassPollerStatePreAuth] = picopass_poller_pre_auth_handler,
    [PicopassPollerStateCheckSecurity] = picopass_poller_check_security,
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

    instance->tx_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);
    instance->rx_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);
    instance->tmp_buffer = bit_buffer_alloc(PICOPASS_POLLER_BUFFER_SIZE);

    return instance;
}

void picopass_poller_free(PicopassPoller* instance) {
    furi_assert(instance);

    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);
    bit_buffer_free(instance->tmp_buffer);
    free(instance);
}

const PicopassData* picopass_poller_get_data(PicopassPoller* instance) {
    furi_assert(instance);

    return &instance->data;
}
