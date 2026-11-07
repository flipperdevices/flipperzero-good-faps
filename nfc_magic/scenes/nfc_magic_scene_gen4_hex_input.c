#include "../nfc_magic_app_i.h"

static const char* nfc_magic_scene_gen4_hex_input_get_header(enum Gen4HexInputMode mode) {
    switch(mode) {
    case Gen4HexInputModeUID:
        return "Enter UID in hex";
    case Gen4HexInputModeATQASAK:
        return "Enter ATQA/SAK (3 bytes)";
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
        instance->gen4_byte_input_store,
        count);
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewByteInput);
}

bool nfc_magic_scene_gen4_hex_input_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicAppCustomEventByteInputDone) {
            uint32_t state =
                scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4HexInput);
            enum Gen4HexInputMode mode = (enum Gen4HexInputMode)(state & 0xFF);
            uint8_t count = (uint8_t)((state >> 8) & 0xFF);
            instance->gen4_input_count = count;

            switch(mode) {
            case Gen4HexInputModeUID:
                instance->gen4_op = Gen4OpWriteUID;
                break;
            case Gen4HexInputModeATQASAK:
                instance->gen4_op = Gen4OpSetATQASAK;
                break;
            case Gen4HexInputModeATS:
                instance->gen4_op = Gen4OpSetATS;
                break;
            case Gen4HexInputModeNTAGPwd:
                instance->gen4_op = Gen4OpWriteNTAGPwd;
                break;
            case Gen4HexInputModePACK:
                instance->gen4_op = Gen4OpWritePACK;
                break;
            case Gen4HexInputModeOTP:
                instance->gen4_op = Gen4OpWriteOTP;
                break;
            case Gen4HexInputModeVersion:
                instance->gen4_op = Gen4OpWriteVersion;
                break;
            case Gen4HexInputModeSignature:
                instance->gen4_op = Gen4OpWriteSignature;
                break;
            case Gen4HexInputModeMaxRWBlock:
                instance->gen4_op = Gen4OpSetMaxRWBlock;
                break;
            }
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4RunOp);
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
