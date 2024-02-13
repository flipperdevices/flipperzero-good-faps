#include "../nfc_magic_app_i.h"

void nfc_magic_check_worker_callback(NfcMagicScannerEvent event, void* context) {
    furi_assert(context);

    NfcMagicApp* instance = context;

    if(event.type == NfcMagicScannerEventTypeDetected) {
        instance->protocol = event.data.protocol;
        if(instance->protocol == NfcMagicProtocolGen2 ||
           instance->protocol == NfcMagicProtocolClassic) {
            instance->poller = nfc_poller_alloc(instance->nfc, NfcProtocolMfClassic);
            if(nfc_poller_detect(instance->poller)) {
                nfc_device_set_data(
                    instance->target_dev,
                    NfcProtocolMfClassic,
                    nfc_poller_get_data(instance->poller));
            } else {
                nfc_poller_free(instance->poller);
                view_dispatcher_send_custom_event(
                    instance->view_dispatcher, NfcMagicCustomEventWorkerFail);
                return;
            }
        }
        nfc_poller_free(instance->poller);
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventWorkerSuccess);
    } else if(event.type == NfcMagicScannerEventTypeDetectedNotMagic) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventWorkerFail);
    }
}

void nfc_magic_scene_check_on_enter(void* context) {
    NfcMagicApp* instance = context;

    popup_set_icon(instance->popup, 0, 8, &I_NFC_manual_60x50);
    popup_set_text(instance->popup, "Apply card to\nthe back", 128, 32, AlignRight, AlignCenter);

    nfc_magic_app_blink_start(instance);

    nfc_magic_scanner_start(instance->scanner, nfc_magic_check_worker_callback, instance);
    nfc_magic_scanner_set_gen4_password(instance->scanner, instance->gen4_password);

    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewPopup);
}

bool nfc_magic_scene_check_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicCustomEventWorkerSuccess) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneMagicInfo);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventWorkerFail) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneNotMagic);
            consumed = true;
        }
    }

    return consumed;
}

void nfc_magic_scene_check_on_exit(void* context) {
    NfcMagicApp* instance = context;

    nfc_magic_scanner_stop(instance->scanner);
    popup_reset(instance->popup);
    nfc_magic_app_blink_stop(instance);
}
