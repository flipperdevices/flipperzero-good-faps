#include "../nfc_magic_app_i.h"

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

static const char* nfc_magic_scene_gen4_hex_input_get_header(enum Gen4HexInputMode mode) {
    switch(mode) {
    case Gen4HexInputModeUID:
        return "Enter UID in hex";
    case Gen4HexInputModeATQASAK:
        return "Enter ATQA/SAK (6 hex)";
    case Gen4HexInputModeATS:
        return "Enter ATS in hex";
    case Gen4HexInputModeNTAGPwd:
        return "Enter NTAG PWD (4B)";
    case Gen4HexInputModePACK:
        return "Enter PACK (2 bytes)";
    case Gen4HexInputModeOTP:
        return "Enter OTP (4 bytes)";
    case Gen4HexInputModeVersion:
        return "Enter Version (8B)";
    case Gen4HexInputModeSignature:
        return "Enter Signature (32B)";
    case Gen4HexInputModeMaxRWBlock:
        return "Enter Max Block (1B)";
    case Gen4HexInputModeProtocol:
        return "Protocol: 00=MC,01=UL";
    case Gen4HexInputModeULMode:
        return "UL Mode: 00-03 hex";
    case Gen4HexInputModeTagType:
        return "Tag Type: 01-25 hex";
    case Gen4HexInputModeWipeType:
        return "Wipe: 00=MFC,01=UL";
    default:
        return "Enter data in hex";
    }
}

void nfc_magic_scene_gen4_hex_input_byte_input_callback(void* context) {
    NfcMagicApp* instance = context;

    view_dispatcher_send_custom_event(
        instance->view_dispatcher, NfcMagicAppCustomEventByteInputDone);
}

void nfc_magic_scene_gen4_hex_input_on_enter(void* context) {
    NfcMagicApp* instance = context;

    uint32_t state =
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4HexInput);
    enum Gen4HexInputMode mode = (enum Gen4HexInputMode)(state & 0xFF);
    uint8_t count = (uint8_t)((state >> 8) & 0xFF);

    ByteInput* byte_input = instance->byte_input;
    byte_input_set_header_text(byte_input, nfc_magic_scene_gen4_hex_input_get_header(mode));
    byte_input_set_result_callback(
        byte_input,
        nfc_magic_scene_gen4_hex_input_byte_input_callback,
        NULL,
        instance,
        instance->byte_input_store,
        count);
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewByteInput);
}

bool nfc_magic_scene_gen4_hex_input_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicAppCustomEventByteInputDone) {
            uint32_t state = scene_manager_get_scene_state(
                instance->scene_manager, NfcMagicSceneGen4HexInput);
            enum Gen4HexInputMode mode = (enum Gen4HexInputMode)(state & 0xFF);
            uint8_t count = (uint8_t)((state >> 8) & 0xFF);
            instance->gen4_input_count = count;

            switch(mode) {
            case Gen4HexInputModeUID:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4WriteUID);
                break;
            case Gen4HexInputModeATQASAK:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetATQASAK);
                break;
            case Gen4HexInputModeATS:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4SetATS);
                break;
            case Gen4HexInputModeNTAGPwd:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4WriteNTAGPwd);
                break;
            case Gen4HexInputModePACK:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4WritePack);
                break;
            case Gen4HexInputModeOTP:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4WriteOTP);
                break;
            case Gen4HexInputModeVersion:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4WriteVersion);
                break;
            case Gen4HexInputModeSignature:
                scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4WriteSignature);
                break;
            case Gen4HexInputModeMaxRWBlock:
                scene_manager_next_scene(
                    instance->scene_manager, NfcMagicSceneGen4SetMaxRWBlock);
                break;
            case Gen4HexInputModeProtocol:
                scene_manager_next_scene(
                    instance->scene_manager, NfcMagicSceneGen4SetProtocol);
                break;
            case Gen4HexInputModeULMode:
                scene_manager_next_scene(
                    instance->scene_manager, NfcMagicSceneGen4SetULMode);
                break;
            case Gen4HexInputModeTagType:
                scene_manager_next_scene(
                    instance->scene_manager, NfcMagicSceneGen4SetType);
                break;
            case Gen4HexInputModeWipeType:
                scene_manager_next_scene(
                    instance->scene_manager, NfcMagicSceneGen4FullWipe);
                break;
            }
            consumed = true;
        }
    }
    return consumed;
}

void nfc_magic_scene_gen4_hex_input_on_exit(void* context) {
    NfcMagicApp* instance = context;

    byte_input_set_result_callback(instance->byte_input, NULL, NULL, NULL, NULL, 0);
    byte_input_set_header_text(instance->byte_input, "");
}
