#include "eink_goodisplay_i.h"
#include "eink_goodisplay_config.h"
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <nfc/protocols/iso14443_4a/iso14443_4a_poller_i.h>
#include <nfc/protocols/iso14443_4a/iso14443_4a_poller.h>

#define TAG "GD_Poller"

typedef NfcCommand (*EinkGoodisplayStateHandler)(Iso14443_3aPoller* poller, NfcEinkScreen* screen);
typedef bool (*GoodisplaySendDataResponseValidatorHandler)(
    const uint8_t* response,
    const size_t response_len);

static bool eink_goodisplay_validate_response(
    const uint8_t* expected,
    const size_t expected_len,
    const uint8_t* response,
    const size_t response_len) {
    furi_assert(response);
    furi_assert(expected);
    if(expected_len != response_len) {
        return false;
    }
    return memcmp(expected, response, response_len) == 0;
}

static bool
    eink_goodisplay_validate_C2_response(const uint8_t* response, const size_t response_len) {
    const uint8_t exp_res[] = {0xC2};
    return eink_goodisplay_validate_response(exp_res, sizeof(exp_res), response, response_len);
}

static bool
    eink_goodisplay_validate_0x02_cmd_response(const uint8_t* response, const size_t response_len) {
    const uint8_t exp_res[] = {0x02, 0x90, 0x00};
    return eink_goodisplay_validate_response(exp_res, sizeof(exp_res), response, response_len);
}

static bool
    eink_goodisplay_validate_0x03_cmd_response(const uint8_t* response, const size_t response_len) {
    const uint8_t exp_res[] = {0x03, 0x90, 0x00};
    return eink_goodisplay_validate_response(exp_res, sizeof(exp_res), response, response_len);
}

static bool eink_goodisplay_validate_read_fid_file_response(
    const uint8_t* response,
    const size_t response_len) {
    furi_assert(response);
    furi_assert(response_len > 0);
    return response[0] == 0x02;
}

static bool
    eink_goodisplay_validate_0xB3_response(const uint8_t* response, const size_t response_len) {
    const uint8_t exp_res[] = {0xA2};
    return eink_goodisplay_validate_response(exp_res, sizeof(exp_res), response, response_len);
}

static bool
    eink_goodisplay_validate_0xB2_response(const uint8_t* response, const size_t response_len) {
    const uint8_t exp_res[] = {0xA3};
    return eink_goodisplay_validate_response(exp_res, sizeof(exp_res), response, response_len);
}

#define eink_goodisplay_validate_0x13_response eink_goodisplay_validate_0xB2_response

static bool eink_goodisplay_validate_update_cmd_response(
    const uint8_t* response,
    const size_t response_len) {
    const uint8_t exp_res[] = {0xF2, 0x01};
    return eink_goodisplay_validate_response(exp_res, sizeof(exp_res), response, response_len);
}

static bool eink_goodisplay_send_data(
    Iso14443_3aPoller* poller,
    const NfcEinkScreen* screen,
    const uint8_t* data,
    const size_t data_len,
    const GoodisplaySendDataResponseValidatorHandler validator) {
    furi_assert(poller);
    furi_assert(screen);
    furi_assert(data);
    furi_assert(data_len > 0);
    furi_assert(validator);

    bit_buffer_reset(screen->tx_buf);
    bit_buffer_reset(screen->rx_buf);

    bit_buffer_append_bytes(screen->tx_buf, data, data_len);

    Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
        poller, screen->tx_buf, screen->rx_buf, 0 /* EINK_SCREEN_ISO14443_POLLER_FWT_FC */);

    bool result = false;
    if(error == Iso14443_3aErrorNone) {
        size_t response_len = bit_buffer_get_size_bytes(screen->rx_buf);
        const uint8_t* response = bit_buffer_get_data(screen->rx_buf);
        result = validator(response, response_len);
    } else {
        FURI_LOG_E(TAG, "Iso14443_3aError: %02X", error);
    }
    return result;
}

static NfcCommand eink_goodisplay_C2(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Send 0xC2");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t data[] = {0xC2};
        bool result = eink_goodisplay_send_data(
            poller, screen, data, sizeof(data), eink_goodisplay_validate_C2_response);

        if(!result) break;

        ctx->poller_state = EinkGoodisplayPollerStateSelectNDEFTagApp;
    } while(false);

    return NfcCommandReset;
}

static NfcCommand
    eink_goodisplay_select_application(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Select NDEF APP");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t data[] = {
            0x02, 0x00, 0xa4, 0x04, 0x00, 0x07, 0xd2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00};
        bool result = eink_goodisplay_send_data(
            poller, screen, data, sizeof(data), eink_goodisplay_validate_0x02_cmd_response);

        if(!result) break;

        eink_goodisplay_on_target_detected(screen);
        ctx->poller_state = EinkGoodisplayPollerStateSelectNDEFFile;
    } while(false);
    return NfcCommandContinue;
}

static NfcCommand
    eink_goodisplay_select_ndef_file(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Select NDEF File");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t data[] = {0x03, 0x00, 0xa4, 0x00, 0x0c, 0x02, 0xe1, 0x03};
        bool result = eink_goodisplay_send_data(
            poller, screen, data, sizeof(data), eink_goodisplay_validate_0x03_cmd_response);

        if(!result) break;

        ctx->poller_state = EinkGoodisplayPollerStateReadFIDFileData;
    } while(false);

    return NfcCommandContinue;
}

static NfcCommand eink_goodisplay_read_fid_file(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Read FID");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t data[] = {0x02, 0x00, 0xb0, 0x00, 0x00, 0x0f};
        bool result = eink_goodisplay_send_data(
            poller, screen, data, sizeof(data), eink_goodisplay_validate_read_fid_file_response);

        if(!result) break;

        ctx->poller_state = EinkGoodisplayPollerStateSelect0xE104File;
    } while(false);

    return NfcCommandContinue;
}

static NfcCommand
    eink_goodisplay_select_E104_file(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Select 0xE104");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t data[] = {0x03, 0x00, 0xa4, 0x00, 0x0c, 0x02, 0xe1, 0x04};
        bool result = eink_goodisplay_send_data(
            poller, screen, data, sizeof(data), eink_goodisplay_validate_0x03_cmd_response);

        if(!result) break;

        ctx->poller_state = EinkGoodisplayPollerStateRead0xE104FileData;
    } while(false);
    return NfcCommandContinue;
}

static NfcCommand
    eink_goodisplay_read_0xE104_file(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Read 0xE104");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t data[] = {0x02, 0x00, 0xb0, 0x00, 0x00, 0x02};
        bool result = eink_goodisplay_send_data(
            poller, screen, data, sizeof(data), eink_goodisplay_validate_read_fid_file_response);

        if(!result) break;

        ctx->poller_state = EinkGoodisplayPollerStateDuplicateC2Cmd;
    } while(false);

    return NfcCommandContinue;
}

static NfcCommand eink_goodisplay_duplicate_C2(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    NfcCommand cmd = eink_goodisplay_C2(poller, screen);

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateDuplicateC2Cmd;

    static uint8_t cnt = 0;
    cnt++;
    if(cnt == 2) {
        ctx->poller_state = EinkGoodisplayPollerStateSendConfigCmd;
        cnt = 0;
    }

    return cmd;
}

static NfcCommand
    eink_goodisplay_send_config_cmd(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Send config");

    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;
    do {
        const uint8_t config1[] = {0x02, 0xf0, 0xdb, 0x02, 0x00, 0x00};
        bool result = eink_goodisplay_send_data(
            poller, screen, config1, sizeof(config1), eink_goodisplay_validate_0x02_cmd_response);

        if(!result) break;

        size_t config_length;
        EinkGoodisplayConfigPack* config = eink_goodisplay_config_pack_alloc(&config_length);
        eink_goodisplay_config_pack_set_by_screen_info(config, &screen->data->base);

        result = eink_goodisplay_send_data(
            poller,
            screen,
            (uint8_t*)config,
            config_length,
            eink_goodisplay_validate_0x03_cmd_response);

        NfcEinkGoodisplayScreenResolution resolution =
            config->eink_size_config.screen_data.screen_resolution;
        NfcEinkGoodisplayScreenChannel channel =
            config->eink_size_config.screen_data.screen_channel;
        eink_goodisplay_config_pack_free(config);

        if(!result) break;

        uint8_t config3[] = {0x02, 0xf0, 0xda, 0x00, 0x00, 0x03, 0xf0, resolution, channel};

        result = eink_goodisplay_send_data(
            poller, screen, config3, sizeof(config3), eink_goodisplay_validate_0x02_cmd_response);

        if(!result) break;

        const uint8_t cmdb3[] = {0xb3};
        result = eink_goodisplay_send_data(
            poller, screen, cmdb3, sizeof(cmdb3), eink_goodisplay_validate_0xB3_response);

        if(!result) break;
        ctx->poller_state = EinkGoodisplayPollerStateSendDataCmd;

    } while(false);

    return NfcCommandContinue;
}

static bool eink_goodisplay_send_image_data(
    Iso14443_3aPoller* poller,
    const NfcEinkScreen* screen,
    const uint8_t* command_header,
    const size_t command_header_length,
    const uint8_t* data,
    const size_t data_len,
    const GoodisplaySendDataResponseValidatorHandler validator) {
    furi_assert(command_header);
    furi_assert(validator);
    furi_assert(poller);
    furi_assert(screen);

    size_t total_size = command_header_length + data_len;
    uint8_t* buf = malloc(total_size);
    memcpy(buf, command_header, command_header_length);
    memcpy(buf + command_header_length, data, data_len);

    bool result = eink_goodisplay_send_data(poller, screen, buf, total_size, validator);

    free(buf);
    return result;
}

static NfcCommand eink_goodisplay_send_image(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    const NfcEinkScreenData* data = screen->data;
    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;

    bool status = true;
    do {
        FURI_LOG_D(TAG, "Send data: %d", ctx->block_number);

        uint8_t header[] = {0x13, 0xf0, 0xd2, 0x00, 0x00, 0xFA};
        header[4] = ctx->block_number;
        status &= eink_goodisplay_send_image_data(
            poller,
            screen,
            header,
            sizeof(header),
            &data->image_data[ctx->data_index],
            data->base.data_block_size - 2,
            eink_goodisplay_validate_0x13_response);

        ctx->data_index += data->base.data_block_size - 2;

        if(!status) {
            ctx->poller_state = EinkGoodisplayPollerStateError;
            break;
        }

        uint8_t header2[] = {0x02};
        status &= eink_goodisplay_send_image_data(
            poller,
            screen,
            header2,
            sizeof(header2),
            &data->image_data[ctx->data_index],
            2,
            eink_goodisplay_validate_0x02_cmd_response);
        ctx->data_index += 2;

        if(!status) {
            ctx->poller_state = EinkGoodisplayPollerStateError;
            break;
        }

        ctx->block_number++;
        ctx->poller_state = (ctx->block_number == screen->device->block_total) ?
                                EinkGoodisplayPollerStateApplyImage :
                                EinkGoodisplayPollerStateSendDataCmd;
    } while(false);
    eink_goodisplay_on_block_processed(screen);
    screen->device->block_current = ctx->block_number;

    return NfcCommandContinue;
}

static NfcCommand eink_goodisplay_begin_update(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateError;

    const uint8_t apply[] = {0x03, 0xf0, 0xd4, 0x05, 0x80, 0x00};
    FURI_LOG_D(TAG, "Applying...");

    bool result = eink_goodisplay_send_data(
        poller, screen, apply, sizeof(apply), eink_goodisplay_validate_update_cmd_response);

    if(result) {
        eink_goodisplay_on_updating(screen);
        ctx->poller_state = EinkGoodisplayPollerStateWaitUpdateDone;
    }

    return NfcCommandContinue;
}

static NfcCommand
    eink_goodisplay_wait_update_done(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
    ctx->poller_state = EinkGoodisplayPollerStateWaitUpdateDone;
    FURI_LOG_D(TAG, "Updating...");

    const uint8_t update[] = {0xF2, 0x01};
    bool result = eink_goodisplay_send_data(
        poller, screen, update, sizeof(update), eink_goodisplay_validate_0x03_cmd_response);
    furi_delay_ms(1500);

    if(result) {
        ctx->poller_state = EinkGoodisplayPollerStateSendDataDone;
    }
    return NfcCommandContinue;
}

static NfcCommand
    eink_goodisplay_finish_handler(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_D(TAG, "Finish");

    uint8_t cnt = 0;
    const uint8_t finish_cmd_cnt = 20;
    do {
        const uint8_t b2[] = {0xb2};
        bool result = eink_goodisplay_send_data(
            poller, screen, b2, sizeof(b2), eink_goodisplay_validate_0xB2_response);

        if(!result) break;
        cnt++;
    } while(cnt < finish_cmd_cnt);
    eink_goodisplay_on_done(screen);
    iso14443_3a_poller_halt(poller);
    return NfcCommandStop;
}

static NfcCommand eink_goodisplay_error_handler(Iso14443_3aPoller* poller, NfcEinkScreen* screen) {
    FURI_LOG_E(TAG, "Error!");
    eink_goodisplay_on_error(screen);
    iso14443_3a_poller_halt(poller);
    return NfcCommandStop;
}

static const EinkGoodisplayStateHandler handlers[EinkGoodisplayPollerStateNum] = {
    [EinkGoodisplayPollerStateSendC2Cmd] = eink_goodisplay_C2,
    [EinkGoodisplayPollerStateSelectNDEFTagApp] = eink_goodisplay_select_application,
    [EinkGoodisplayPollerStateSelectNDEFFile] = eink_goodisplay_select_ndef_file,
    [EinkGoodisplayPollerStateReadFIDFileData] = eink_goodisplay_read_fid_file,
    [EinkGoodisplayPollerStateSelect0xE104File] = eink_goodisplay_select_E104_file,
    [EinkGoodisplayPollerStateRead0xE104FileData] = eink_goodisplay_read_0xE104_file,
    [EinkGoodisplayPollerStateDuplicateC2Cmd] = eink_goodisplay_duplicate_C2,
    [EinkGoodisplayPollerStateSendConfigCmd] = eink_goodisplay_send_config_cmd,
    [EinkGoodisplayPollerStateSendDataCmd] = eink_goodisplay_send_image,
    [EinkGoodisplayPollerStateApplyImage] = eink_goodisplay_begin_update,
    [EinkGoodisplayPollerStateWaitUpdateDone] = eink_goodisplay_wait_update_done,
    [EinkGoodisplayPollerStateSendDataDone] = eink_goodisplay_finish_handler,
    [EinkGoodisplayPollerStateError] = eink_goodisplay_error_handler,
};

NfcCommand eink_goodisplay_poller_callback(NfcGenericEvent event, void* context) {
    furi_assert(context);
    NfcEinkScreen* screen = context;

    NfcCommand command = NfcCommandContinue;

    Iso14443_4aPollerEvent* Iso14443_4a_event = event.event_data;
    Iso14443_4aPoller* poller = event.instance;

    if(Iso14443_4a_event->type == Iso14443_4aPollerEventTypeReady) {
        //bit_buffer_reset(screen->tx_buf);
        //bit_buffer_reset(screen->rx_buf);

        NfcEinkScreenSpecificGoodisplayContext* ctx = screen->device->screen_context;
        command = handlers[ctx->poller_state](poller->iso14443_3a_poller, screen);
    }
    if(command == NfcCommandReset) iso14443_4a_poller_halt(poller);
    return command;
}
