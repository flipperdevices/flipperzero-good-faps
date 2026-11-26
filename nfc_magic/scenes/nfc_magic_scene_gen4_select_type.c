#include "../nfc_magic_app_i.h"

enum SubmenuIndex {
    SubmenuIndexType01,
    SubmenuIndexType02,
    SubmenuIndexType03,
    SubmenuIndexType04,
    SubmenuIndexType05,
    SubmenuIndexType06,
    SubmenuIndexType07,
    SubmenuIndexType08,
    SubmenuIndexType09,
    SubmenuIndexType10,
    SubmenuIndexType11,
    SubmenuIndexType12,
    SubmenuIndexType13,
    SubmenuIndexType15,
    SubmenuIndexType16,
    SubmenuIndexType17,
    SubmenuIndexType18,
    SubmenuIndexType19,
    SubmenuIndexType20,
    SubmenuIndexType21,
    SubmenuIndexType22,
    SubmenuIndexType23,
    SubmenuIndexType24,
    SubmenuIndexType25,
};

void nfc_magic_scene_gen4_select_type_submenu_callback(void* context, uint32_t index) {
    NfcMagicApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_magic_scene_gen4_select_type_on_enter(void* context) {
    NfcMagicApp* instance = context;

    Submenu* submenu = instance->submenu;
    submenu_add_item(
        submenu,
        "Mifare Mini S20 4B",
        SubmenuIndexType01,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare Mini S20 7B",
        SubmenuIndexType02,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare Mini S20 10B",
        SubmenuIndexType03,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare 1k S50 4B",
        SubmenuIndexType04,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare 1k S50 7B",
        SubmenuIndexType05,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare 1k S50 10B",
        SubmenuIndexType06,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare 4k S70 4B",
        SubmenuIndexType07,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare 4k S70 7B",
        SubmenuIndexType08,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Mifare 4k S70 10B",
        SubmenuIndexType09,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Ultralight (not working yet)",
        SubmenuIndexType10,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "UL-C (not working yet)",
        SubmenuIndexType11,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "UL EV1 48b",
        SubmenuIndexType12,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "UL EV1 128b",
        SubmenuIndexType13,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 210",
        SubmenuIndexType15,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 212",
        SubmenuIndexType16,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 213",
        SubmenuIndexType17,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 215",
        SubmenuIndexType18,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 216",
        SubmenuIndexType19,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG I2C 1K",
        SubmenuIndexType20,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG I2C 2K",
        SubmenuIndexType21,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG I2C 1K PLUS",
        SubmenuIndexType22,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG I2C 2K PLUS",
        SubmenuIndexType23,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 213F",
        SubmenuIndexType24,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "NTAG 216F",
        SubmenuIndexType25,
        nfc_magic_scene_gen4_select_type_submenu_callback,
        instance);

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SelectType));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_select_type_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event <= SubmenuIndexType25) {
            uint8_t tag_type = (event.event <= SubmenuIndexType13) ? event.event + 1 :
                                                                     event.event + 2;
            instance->gen4_op = Gen4OpSetType;
            instance->gen4_op_param = tag_type;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4RunOp);
            consumed = true;
        }

        scene_manager_set_scene_state(
            instance->scene_manager, NfcMagicSceneGen4SelectType, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneGen4Advanced);
    }

    return consumed;
}

void nfc_magic_scene_gen4_select_type_on_exit(void* context) {
    NfcMagicApp* instance = context;
    submenu_reset(instance->submenu);
}
