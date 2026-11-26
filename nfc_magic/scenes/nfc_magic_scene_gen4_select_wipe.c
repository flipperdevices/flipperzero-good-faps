#include "../nfc_magic_app_i.h"

enum SubmenuIndex {
    SubmenuIndexWipeMFC,
    SubmenuIndexWipeUL,
};

void nfc_magic_scene_gen4_select_wipe_submenu_callback(void* context, uint32_t index) {
    NfcMagicApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_magic_scene_gen4_select_wipe_on_enter(void* context) {
    NfcMagicApp* instance = context;

    Submenu* submenu = instance->submenu;
    submenu_add_item(
        submenu,
        "MIFARE Classic Wipe",
        SubmenuIndexWipeMFC,
        nfc_magic_scene_gen4_select_wipe_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Ultralight Wipe",
        SubmenuIndexWipeUL,
        nfc_magic_scene_gen4_select_wipe_submenu_callback,
        instance);

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SelectWipe));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_select_wipe_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexWipeMFC) {
            instance->gen4_op = Gen4OpFullWipe;
            instance->gen4_op_param = 0;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4RunOp);
            consumed = true;
        } else if(event.event == SubmenuIndexWipeUL) {
            instance->gen4_op = Gen4OpFullWipe;
            instance->gen4_op_param = 1;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4RunOp);
            consumed = true;
        }
        scene_manager_set_scene_state(
            instance->scene_manager, NfcMagicSceneGen4SelectWipe, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneGen4Advanced);
    }
    return consumed;
}

void nfc_magic_scene_gen4_select_wipe_on_exit(void* context) {
    NfcMagicApp* instance = context;
    submenu_reset(instance->submenu);
}
