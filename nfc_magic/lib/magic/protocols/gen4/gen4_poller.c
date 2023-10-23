#include "gen4_poller_i.h"
#include <nfc/protocols/iso14443_3a/iso14443_3a.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <nfc/helpers/nfc_util.h>
#include <nfc/nfc_poller.h>

#include <furi/furi.h>

#define GEN4_POLLER_THREAD_FLAG_DETECTED (1U << 0)

typedef NfcCommand (*Gen4PollerStateHandler)(Gen4Poller* instance);

typedef struct {
    NfcPoller* poller;
    uint32_t password;
    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;
    FuriThreadId thread_id;
    bool detected;
} Gen4PollerDetectContext;

Gen4Poller* gen4_poller_alloc(Nfc* nfc) {
    furi_assert(nfc);

    Gen4Poller* instance = malloc(sizeof(Gen4Poller));
    nfc_poller_alloc(nfc, NfcProtocolIso14443_3a);

    instance->gen4_event.data = &instance->gen4_event_data;

    instance->tx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);
    instance->rx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);

    return instance;
}

void gen4_poller_free(Gen4Poller* instance) {
    furi_assert(instance);

    nfc_poller_free(instance->poller);

    bit_buffer_free(instance->tx_buffer);
    bit_buffer_free(instance->rx_buffer);

    free(instance);
}

void gen4_poller_set_password(Gen4Poller* instance, uint32_t password) {
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

    if(iso3_event->type == Iso14443_3aPollerEventTypeReady) {
        do {
            bit_buffer_append_byte(gen4_poller_detect_ctx->tx_buffer, GEN4_CMD_PREFIX);
            uint8_t pwd_arr[4] = {};
            nfc_util_num2bytes(gen4_poller_detect_ctx->password, COUNT_OF(pwd_arr), pwd_arr);
            bit_buffer_append_bytes(gen4_poller_detect_ctx->tx_buffer, pwd_arr, COUNT_OF(pwd_arr));
            bit_buffer_append_byte(gen4_poller_detect_ctx->tx_buffer, GEN4_CMD_GET_CFG);

            Iso14443_3aError error = iso14443_3a_poller_send_standard_frame(
                iso3_poller,
                gen4_poller_detect_ctx->tx_buffer,
                gen4_poller_detect_ctx->rx_buffer,
                GEN4_POLLER_MAX_FWT);

            if(error != Iso14443_3aErrorNone) break;
            size_t rx_bytes = bit_buffer_get_size_bytes(gen4_poller_detect_ctx->rx_buffer);
            if((rx_bytes != 30) && (rx_bytes != 32)) break;

            gen4_poller_detect_ctx->detected = true;
        } while(false);
    }
    furi_thread_flags_set(gen4_poller_detect_ctx->thread_id, GEN4_POLLER_THREAD_FLAG_DETECTED);

    return command;
}

bool gen4_poller_detect(Nfc* nfc, uint32_t password) {
    furi_assert(nfc);

    Gen4PollerDetectContext gen4_poller_detect_ctx = {};
    gen4_poller_detect_ctx.poller = nfc_poller_alloc(nfc, NfcProtocolIso14443_3a);
    gen4_poller_detect_ctx.password = password;
    gen4_poller_detect_ctx.tx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);
    gen4_poller_detect_ctx.rx_buffer = bit_buffer_alloc(GEN4_POLLER_MAX_BUFFER_SIZE);
    gen4_poller_detect_ctx.thread_id = furi_thread_get_current_id();
    gen4_poller_detect_ctx.detected = false;

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

    return gen4_poller_detect_ctx.detected;
}

NfcCommand gen4_poller_idle_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;

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
        instance->state = Gen4PollerStateWriteDataRequest;
    } else {
        instance->state = Gen4PollerStateFail;
    }

    return command;
}

NfcCommand gen4_poller_wipe_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    UNUSED(instance);

    return command;
}

NfcCommand gen4_poller_write_data_request_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    UNUSED(instance);

    return command;
}

NfcCommand gen4_poller_write_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    UNUSED(instance);

    return command;
}

NfcCommand gen4_poller_success_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    UNUSED(instance);

    return command;
}

NfcCommand gen4_poller_fail_handler(Gen4Poller* instance) {
    NfcCommand command = NfcCommandContinue;
    UNUSED(instance);

    return command;
}

static const Gen4PollerStateHandler gen4_poller_state_handlers[Gen4PollerStateNum] = {
    [Gen4PollerStateIdle] = gen4_poller_idle_handler,
    [Gen4PollerStateRequestMode] = gen4_poller_request_mode_handler,
    [Gen4PollerStateWipe] = gen4_poller_wipe_handler,
    [Gen4PollerStateWriteDataRequest] = gen4_poller_write_data_request_handler,
    [Gen4PollerStateWrite] = gen4_poller_write_handler,
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

    if(iso3_event->type == Iso14443_3aPollerEventTypeError) {
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
