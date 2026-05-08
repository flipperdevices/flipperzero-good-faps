#include "../nfc_magic_app_i.h"

enum SubmenuIndex {
    SubmenuIndexSetType = 0,
    SubmenuIndexWriteUID,
    SubmenuIndexSetATQASAK,
    SubmenuIndexSetATS,
    SubmenuIndexSetProtocol,
    SubmenuIndexSetULMode,
    SubmenuIndexSetMaxRWBlock,
    SubmenuIndexWriteNTAGPwd,
    SubmenuIndexWritePACK,
    SubmenuIndexWriteOTP,
    SubmenuIndexWriteVersion,
    SubmenuIndexWriteSignature,
    SubmenuIndexFullWipe,
};

enum Gen4HexInputMode {
    Gen4HexInputModeUID,
    Gen4HexInputModeATQASAK,
    Gen4HexInputModeATS,
    Gen4HexInputModeNTAGPwd,
    Gen4HexInputModePACK,
    Gen4HexInputModeOTP,
    Gen4HexInputModeVersion,
    Gen4HexInputModeSignature,
    Gen4HexInputModeMaxRWBlock,
    Gen4HexInputModeProtocol,
    Gen4HexInputModeULMode,
    Gen4HexInputModeTagType,
    Gen4HexInputModeWipeType,
};

void nfc_magic_scene_gen4_advanced_submenu_callback(void* context, uint32_t index) {
    NfcMagicApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_magic_scene_gen4_advanced_on_enter(void* context) {
    NfcMagicApp* instance = context;

    Submenu* submenu = instance->submenu;
    submenu_add_item(
        submenu,
        "Set Tag Type",
        SubmenuIndexSetType,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Write UID",
        SubmenuIndexWriteUID,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Set ATQA/SAK",
        SubmenuIndexSetATQASAK,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Set ATS",
        SubmenuIndexSetATS,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Set UL Protocol",
        SubmenuIndexSetProtocol,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Set UL Mode",
        SubmenuIndexSetULMode,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Set Max R/W Block",
        SubmenuIndexSetMaxRWBlock,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Write NTAG PWD",
        SubmenuIndexWriteNTAGPwd,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Write PACK",
        SubmenuIndexWritePACK,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Write OTP",
        SubmenuIndexWriteOTP,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Write Version",
        SubmenuIndexWriteVersion,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Write Signature",
        SubmenuIndexWriteSignature,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);
    submenu_add_item(
        submenu,
        "Full Wipe",
        SubmenuIndexFullWipe,
        nfc_magic_scene_gen4_advanced_submenu_callback,
        instance);

    submenu_set_selected_item(
        submenu,
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4Advanced));
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewMenu);
}

bool nfc_magic_scene_gen4_advanced_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexWriteUID) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SelectUIDLen);
            consumed = true;
        } else if(event.event == SubmenuIndexSetATQASAK) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeATQASAK | (3UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexSetATS) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeATS | (17UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexSetProtocol) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SelectProtocol);
            consumed = true;
        } else if(event.event == SubmenuIndexSetULMode) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SelectULMode);
            consumed = true;
        } else if(event.event == SubmenuIndexSetMaxRWBlock) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeMaxRWBlock | (1UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexSetType) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SelectType);
            consumed = true;
        } else if(event.event == SubmenuIndexWriteNTAGPwd) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeNTAGPwd | (4UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexWritePACK) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModePACK | (2UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexWriteOTP) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeOTP | (4UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexWriteVersion) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeVersion | (8UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexWriteSignature) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4HexInput,
                Gen4HexInputModeSignature | (32UL << 8));
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4HexInput);
            consumed = true;
        } else if(event.event == SubmenuIndexFullWipe) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SelectWipe);
            consumed = true;
        }

        scene_manager_set_scene_state(
            instance->scene_manager, NfcMagicSceneGen4Advanced, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneGen4ActionsMenu);
    }

    return consumed;
}

void nfc_magic_scene_gen4_advanced_on_exit(void* context) {
    NfcMagicApp* instance = context;
    scene_manager_set_scene_state(
        instance->scene_manager, NfcMagicSceneGen4Advanced, 0);
    submenu_reset(instance->submenu);
}
