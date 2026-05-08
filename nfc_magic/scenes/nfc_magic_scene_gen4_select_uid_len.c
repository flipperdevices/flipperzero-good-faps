#include "../nfc_magic_app_i.h"

enum {
    Gen4HexInputModeUID = 0,
};

enum SubmenuIndex {
    SubmenuIndexUID4,
    SubmenuIndexUID7,
    SubmenuIndexUID10,
};

void nfc_magic_scene_gen4_select_uid_len_submenu_callback(void* context, uint32_t index) {
    NfcMagicApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_magic_scene_gen4_select_uid_len_on_enter(void* context) {
    NfcMagicApp* instance = context;

    Submenu* submenu = instance->submenu;
    submenu_add_item(
        submenu, "4-byte UID", SubmenuIndexUID4,
        nfc_magic_scene_gen4_select_uid_len_submenu_callback, instance);
    submenu_add_item(
        submenu, "7-byte UID", SubmenuIndexUID7,
        nfc_magic_scene_gen4_select_uid_len_submenu_callback, instance);
    submenu_add_item(
        submenu, "10-byte UID", SubmenuIndexUID10,
        nfc_magic_scene_gen4_select_uid_len_submenu_callback, instance);

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SelectUIDLen));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_select_uid_len_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint8_t count = 0;
        if(event.event == SubmenuIndexUID4) count = 4;
        else if(event.event == SubmenuIndexUID7) count = 7;
        else if(event.event == SubmenuIndexUID10) count = 10;

        if(count) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeUID | ((uint32_t)count << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        }

        scene_manager_set_scene_state(
            instance->scene_manager, NfcMagicSceneGen4SelectUIDLen, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneGen4Advanced);
    }
    return consumed;
}

void nfc_magic_scene_gen4_select_uid_len_on_exit(void* context) {
    NfcMagicApp* instance = context;
    submenu_reset(instance->submenu);
}
