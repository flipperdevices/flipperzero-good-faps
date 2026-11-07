#include "../nfc_magic_app_i.h"

typedef void (*Gen4OpParamSetter)(Gen4Poller* poller, NfcMagicApp* app);

typedef struct {
    Gen4PollerMode mode;
    const char* header;
    Gen4OpParamSetter set_params;
} Gen4OpDescriptor;

static void gen4_op_set_type_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_tag_type(poller, app->gen4_op_param);
}

static void gen4_op_write_uid_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_uid(poller, app->gen4_byte_input_store, app->gen4_input_count);
}

static void gen4_op_set_atqa_sak_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_atqa_sak_params(
        poller,
        app->gen4_byte_input_store[0],
        app->gen4_byte_input_store[1],
        app->gen4_byte_input_store[2]);
}

static void gen4_op_set_ats_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_ats_params(poller, app->gen4_byte_input_store, app->gen4_input_count);
}

static void gen4_op_set_protocol_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_protocol(poller, app->gen4_op_param);
}

static void gen4_op_set_ul_mode_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_ul_mode(poller, app->gen4_op_param);
}

static void gen4_op_set_max_rw_block_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_max_rw_block(poller, app->gen4_byte_input_store[0]);
}

static void gen4_op_write_ntag_pwd_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_ntag_pwd(poller, app->gen4_byte_input_store);
}

static void gen4_op_write_pack_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_pack(poller, app->gen4_byte_input_store);
}

static void gen4_op_write_otp_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_otp(poller, app->gen4_byte_input_store);
}

static void gen4_op_write_version_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_version(poller, app->gen4_byte_input_store);
}

static void gen4_op_write_signature_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_signature(poller, app->gen4_byte_input_store);
}

static void gen4_op_full_wipe_params(Gen4Poller* poller, NfcMagicApp* app) {
    gen4_poller_struct_set_wipe_type(poller, app->gen4_op_param);
}

static const Gen4OpDescriptor gen4_op_descriptors[Gen4OpCount] = {
    [Gen4OpSetType] =
        {Gen4PollerModeSetType, "Setting Type\nDon't move...", gen4_op_set_type_params},
    [Gen4OpWriteUID] =
        {Gen4PollerModeWriteUID, "Writing UID\nDon't move...", gen4_op_write_uid_params},
    [Gen4OpSetATQASAK] =
        {Gen4PollerModeSetATQASAK, "Setting ATQA/SAK\nDon't move...", gen4_op_set_atqa_sak_params},
    [Gen4OpSetATS] = {Gen4PollerModeSetATS, "Setting ATS\nDon't move...", gen4_op_set_ats_params},
    [Gen4OpSetProtocol] =
        {Gen4PollerModeSetULProtocol,
         "Setting Protocol\nDon't move...",
         gen4_op_set_protocol_params},
    [Gen4OpSetULMode] =
        {Gen4PollerModeSetULMode, "Setting UL Mode\nDon't move...", gen4_op_set_ul_mode_params},
    [Gen4OpSetMaxRWBlock] =
        {Gen4PollerModeSetMaxRWBlock,
         "Setting Max RW\nDon't move...",
         gen4_op_set_max_rw_block_params},
    [Gen4OpWriteNTAGPwd] =
        {Gen4PollerModeWriteNTAGPwd,
         "Writing NTAG Pwd\nDon't move...",
         gen4_op_write_ntag_pwd_params},
    [Gen4OpWritePACK] =
        {Gen4PollerModeWritePack, "Writing PACK\nDon't move...", gen4_op_write_pack_params},
    [Gen4OpWriteOTP] =
        {Gen4PollerModeWriteOTP, "Writing OTP\nDon't move...", gen4_op_write_otp_params},
    [Gen4OpWriteVersion] =
        {Gen4PollerModeWriteVersion,
         "Writing Version\nDon't move...",
         gen4_op_write_version_params},
    [Gen4OpWriteSignature] =
        {Gen4PollerModeWriteSignature,
         "Writing Signature\nDon't move...",
         gen4_op_write_signature_params},
    [Gen4OpFullWipe] =
        {Gen4PollerModeFullWipe, "Full Wipe\nDon't move...", gen4_op_full_wipe_params},
};

enum {
    NfcMagicSceneGen4RunOpStateCardSearch,
    NfcMagicSceneGen4RunOpStateCardFound,
};

NfcCommand nfc_magic_scene_gen4_run_op_poller_callback(Gen4PollerEvent event, void* context) {
    NfcMagicApp* instance = context;
    furi_assert(event.data);

    NfcCommand command = NfcCommandContinue;
    if(event.type == Gen4PollerEventTypeCardDetected) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventCardDetected);
    } else if(event.type == Gen4PollerEventTypeRequestMode) {
        event.data->request_mode.mode = gen4_op_descriptors[instance->gen4_op].mode;
    } else if(event.type == Gen4PollerEventTypeSuccess) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventWorkerSuccess);
        command = NfcCommandStop;
    } else if(event.type == Gen4PollerEventTypeFail) {
        instance->gen4_last_error = event.data->fail.error;
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventWorkerFail);
        command = NfcCommandStop;
    }

    return command;
}

static void nfc_magic_scene_gen4_run_op_setup_view(NfcMagicApp* instance) {
    Popup* popup = instance->popup;
    popup_reset(popup);
    uint32_t state =
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4RunOp);

    if(state == NfcMagicSceneGen4RunOpStateCardSearch) {
        popup_set_icon(instance->popup, 0, 8, &I_NFC_manual_60x50);
        popup_set_text(
            instance->popup, "Apply the\ncard\nto the back", 128, 32, AlignRight, AlignCenter);
    } else {
        popup_set_icon(popup, 12, 23, &I_Loading_24);
        popup_set_header(
            popup, gen4_op_descriptors[instance->gen4_op].header, 52, 32, AlignLeft, AlignCenter);
    }

    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewPopup);
}

void nfc_magic_scene_gen4_run_op_on_enter(void* context) {
    NfcMagicApp* instance = context;
    furi_assert(instance->gen4_op < Gen4OpCount);

    scene_manager_set_scene_state(
        instance->scene_manager, NfcMagicSceneGen4RunOp, NfcMagicSceneGen4RunOpStateCardSearch);
    nfc_magic_scene_gen4_run_op_setup_view(instance);

    nfc_magic_app_blink_start(instance);

    instance->gen4_poller = gen4_poller_alloc(instance->nfc);
    gen4_poller_set_password(instance->gen4_poller, instance->gen4_password);
    gen4_op_descriptors[instance->gen4_op].set_params(instance->gen4_poller, instance);

    gen4_poller_start(
        instance->gen4_poller, nfc_magic_scene_gen4_run_op_poller_callback, instance);
}

bool nfc_magic_scene_gen4_run_op_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicCustomEventCardDetected) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4RunOp,
                NfcMagicSceneGen4RunOpStateCardFound);
            nfc_magic_scene_gen4_run_op_setup_view(instance);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventCardLost) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4RunOp,
                NfcMagicSceneGen4RunOpStateCardSearch);
            nfc_magic_scene_gen4_run_op_setup_view(instance);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventWorkerSuccess) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneSuccess);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventWorkerFail) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4Fail);
            consumed = true;
        }
    }

    return consumed;
}

void nfc_magic_scene_gen4_run_op_on_exit(void* context) {
    NfcMagicApp* instance = context;

    gen4_poller_stop(instance->gen4_poller);
    gen4_poller_free(instance->gen4_poller);
    scene_manager_set_scene_state(
        instance->scene_manager, NfcMagicSceneGen4RunOp, NfcMagicSceneGen4RunOpStateCardSearch);
    popup_reset(instance->popup);

    nfc_magic_app_blink_stop(instance);
}
