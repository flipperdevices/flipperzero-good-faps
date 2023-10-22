#include "../nfc_magic_app_i.h"

void nfc_magic_scene_key_input_byte_input_callback(void* context) {
    NfcMagicApp* instance = context;

    view_dispatcher_send_custom_event(
        instance->view_dispatcher, NfcMagicAppCustomEventByteInputDone);
}

void nfc_magic_scene_key_input_on_enter(void* context) {
    NfcMagicApp* instance = context;

    // Setup view
    ByteInput* byte_input = instance->byte_input;
    byte_input_set_header_text(byte_input, "Enter the password in hex");
    byte_input_set_result_callback(
        byte_input,
        nfc_magic_scene_key_input_byte_input_callback,
        NULL,
        instance,
        (uint8_t*)&instance->gen4_password,
        4);
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewByteInput);
}

bool nfc_magic_scene_key_input_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicAppCustomEventByteInputDone) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneCheck);
            consumed = true;
        }
    }
    return consumed;
}

void nfc_magic_scene_key_input_on_exit(void* context) {
    NfcMagicApp* instance = context;

    // Clear view
    byte_input_set_result_callback(instance->byte_input, NULL, NULL, NULL, NULL, 0);
    byte_input_set_header_text(instance->byte_input, "");
}
