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
        submenu, "Mifare Mini S20 4B", SubmenuIndexType01,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare Mini S20 7B", SubmenuIndexType02,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare Mini S20 10B", SubmenuIndexType03,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare 1k S50 4B", SubmenuIndexType04,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare 1k S50 7B", SubmenuIndexType05,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare 1k S50 10B", SubmenuIndexType06,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare 4k S70 4B", SubmenuIndexType07,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare 4k S70 7B", SubmenuIndexType08,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Mifare 4k S70 10B", SubmenuIndexType09,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "Ultralight (not working yet)", SubmenuIndexType10,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "UL-C (not working yet)", SubmenuIndexType11,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "UL EV1 48b", SubmenuIndexType12,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "UL EV1 128b", SubmenuIndexType13,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 210", SubmenuIndexType15,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 212", SubmenuIndexType16,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 213", SubmenuIndexType17,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 215", SubmenuIndexType18,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 216", SubmenuIndexType19,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG I2C 1K", SubmenuIndexType20,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG I2C 2K", SubmenuIndexType21,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG I2C 1K PLUS", SubmenuIndexType22,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG I2C 2K PLUS", SubmenuIndexType23,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 213F", SubmenuIndexType24,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);
    submenu_add_item(
        submenu, "NTAG 216F", SubmenuIndexType25,
        nfc_magic_scene_gen4_select_type_submenu_callback, instance);

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SelectType));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_select_type_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint8_t tag_type = 0;
        if(event.event == SubmenuIndexType01) tag_type = 1;
        else if(event.event == SubmenuIndexType02) tag_type = 2;
        else if(event.event == SubmenuIndexType03) tag_type = 3;
        else if(event.event == SubmenuIndexType04) tag_type = 4;
        else if(event.event == SubmenuIndexType05) tag_type = 5;
        else if(event.event == SubmenuIndexType06) tag_type = 6;
        else if(event.event == SubmenuIndexType07) tag_type = 7;
        else if(event.event == SubmenuIndexType08) tag_type = 8;
        else if(event.event == SubmenuIndexType09) tag_type = 9;
        else if(event.event == SubmenuIndexType10) tag_type = 10;
        else if(event.event == SubmenuIndexType11) tag_type = 11;
        else if(event.event == SubmenuIndexType12) tag_type = 12;
        else if(event.event == SubmenuIndexType13) tag_type = 13;
        else if(event.event == SubmenuIndexType15) tag_type = 15;
        else if(event.event == SubmenuIndexType16) tag_type = 16;
        else if(event.event == SubmenuIndexType17) tag_type = 17;
        else if(event.event == SubmenuIndexType18) tag_type = 18;
        else if(event.event == SubmenuIndexType19) tag_type = 19;
        else if(event.event == SubmenuIndexType20) tag_type = 20;
        else if(event.event == SubmenuIndexType21) tag_type = 21;
        else if(event.event == SubmenuIndexType22) tag_type = 22;
        else if(event.event == SubmenuIndexType23) tag_type = 23;
        else if(event.event == SubmenuIndexType24) tag_type = 24;
        else if(event.event == SubmenuIndexType25) tag_type = 25;

        if(tag_type) {
            instance->byte_input_store[0] = tag_type;
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetType);
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
