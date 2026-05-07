#include "../nfc_magic_app_i.h"

enum SubmenuIndex {
    SubmenuIndexULEV1,
    SubmenuIndexNTAG,
    SubmenuIndexULC,
    SubmenuIndexUL,
};

void nfc_magic_scene_gen4_select_ul_mode_submenu_callback(void* context, uint32_t index) {
    NfcMagicApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_magic_scene_gen4_select_ul_mode_on_enter(void* context) {
    NfcMagicApp* instance = context;

    Submenu* submenu = instance->submenu;
    submenu_add_item(
        submenu, "UL EV1", SubmenuIndexULEV1,
        nfc_magic_scene_gen4_select_ul_mode_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG", SubmenuIndexNTAG,
        nfc_magic_scene_gen4_select_ul_mode_submenu_callback, instance);
    submenu_add_item(
        submenu, "UL-C", SubmenuIndexULC,
        nfc_magic_scene_gen4_select_ul_mode_submenu_callback, instance);
    submenu_add_item(
        submenu, "Ultralight", SubmenuIndexUL,
        nfc_magic_scene_gen4_select_ul_mode_submenu_callback, instance);

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SelectULMode));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_select_ul_mode_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexULEV1) {
            instance->byte_input_store[0] = 0x00;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetULMode);
            consumed = true;
        } else if(event.event == SubmenuIndexNTAG) {
            instance->byte_input_store[0] = 0x01;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetULMode);
            consumed = true;
        } else if(event.event == SubmenuIndexULC) {
            instance->byte_input_store[0] = 0x02;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetULMode);
            consumed = true;
        } else if(event.event == SubmenuIndexUL) {
            instance->byte_input_store[0] = 0x03;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetULMode);
            consumed = true;
        }
        scene_manager_set_scene_state(
            instance->scene_manager, NfcMagicSceneGen4SelectULMode, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneGen4Advanced);
    }
    return consumed;
}

void nfc_magic_scene_gen4_select_ul_mode_on_exit(void* context) {
    NfcMagicApp* instance = context;
    submenu_reset(instance->submenu);
}
