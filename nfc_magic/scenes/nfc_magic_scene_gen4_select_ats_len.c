#include "../nfc_magic_app_i.h"

enum SubmenuIndex {
    SubmenuIndexAtsLen0,
    SubmenuIndexAtsLen1,
    SubmenuIndexAtsLen2,
    SubmenuIndexAtsLen3,
    SubmenuIndexAtsLen4,
    SubmenuIndexAtsLen5,
    SubmenuIndexAtsLen6,
    SubmenuIndexAtsLen7,
    SubmenuIndexAtsLen8,
    SubmenuIndexAtsLen9,
    SubmenuIndexAtsLen10,
    SubmenuIndexAtsLen11,
    SubmenuIndexAtsLen12,
    SubmenuIndexAtsLen13,
    SubmenuIndexAtsLen14,
    SubmenuIndexAtsLen15,
    SubmenuIndexAtsLen16,
};

void nfc_magic_scene_gen4_select_ats_len_submenu_callback(void* context, uint32_t index) {
    NfcMagicApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_magic_scene_gen4_select_ats_len_on_enter(void* context) {
    NfcMagicApp* instance = context;

    Submenu* submenu = instance->submenu;
    static const char* const ats_len_names[] = {
        "0 - Disable ATS",
        "1 byte",
        "2 bytes",
        "3 bytes",
        "4 bytes",
        "5 bytes",
        "6 bytes",
        "7 bytes",
        "8 bytes",
        "9 bytes",
        "10 bytes",
        "11 bytes",
        "12 bytes",
        "13 bytes",
        "14 bytes",
        "15 bytes",
        "16 bytes",
    };
    for(size_t i = 0; i < COUNT_OF(ats_len_names); i++) {
        submenu_add_item(
            submenu,
            ats_len_names[i],
            i,
            nfc_magic_scene_gen4_select_ats_len_submenu_callback,
            instance);
    }

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SelectAtsLen));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_select_ats_len_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event <= SubmenuIndexAtsLen16) {
            uint8_t ats_len = event.event;
            instance->gen4_op = Gen4OpSetATS;
            instance->gen4_input_count = ats_len;
            if(ats_len == 0) {
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4RunOp);
            } else {
                scene_manager_set_scene_state(
                    instance->scene_manager,
                    NfcMagicSceneGen4HexInput,
                    Gen4HexInputModeATS | ((uint32_t)ats_len << 8));
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            }
            consumed = true;
        }

        scene_manager_set_scene_state(
            instance->scene_manager, NfcMagicSceneGen4SelectAtsLen, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneGen4Advanced);
    }

    return consumed;
}

void nfc_magic_scene_gen4_select_ats_len_on_exit(void* context) {
    NfcMagicApp* instance = context;
    submenu_reset(instance->submenu);
}
