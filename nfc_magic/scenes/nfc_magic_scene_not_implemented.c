#include "../nfc_magic_app_i.h"

void nfc_magic_scene_not_implemented_popup_callback(void* context) {
    NfcMagicApp* instance = context;

    view_dispatcher_send_custom_event(instance->view_dispatcher, NfcMagicCustomEventViewExit);
}

void nfc_magic_scene_not_implemented_on_enter(void* context) {
    NfcMagicApp* instance = context;

    // Setup view
    Popup* popup = instance->popup;
    popup_set_header(popup, "Not implemented!", 64, 32, AlignCenter, AlignCenter);
    popup_set_timeout(popup, 1500);
    popup_set_context(popup, instance);
    popup_set_callback(popup, nfc_magic_scene_not_implemented_popup_callback);
    popup_enable_timeout(popup);
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewPopup);
}

bool nfc_magic_scene_not_implemented_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicCustomEventViewExit) {
            consumed = scene_manager_previous_scene(instance->scene_manager);
        }
    }
    return consumed;
}

void nfc_magic_scene_not_implemented_on_exit(void* context) {
    NfcMagicApp* instance = context;

    // Clear view
    popup_reset(instance->popup);
}
