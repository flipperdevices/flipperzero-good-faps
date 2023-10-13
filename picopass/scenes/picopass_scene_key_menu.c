#include "../picopass_i.h"
#include "../picopass_keys.h"

enum SubmenuIndex {
    SubmenuIndexWriteStandard,
    SubmenuIndexWriteiCE,
    SubmenuIndexWriteiCL,
    SubmenuIndexWriteiCS,
    SubmenuIndexWriteCustom,
};

typedef struct {
    const char* name;
    const uint8_t* key;
    bool is_elite;
} PicopassSceneKeyData;

const PicopassSceneKeyData picopass_scene_key_data[SubmenuIndexWriteCustom] = {
    [SubmenuIndexWriteStandard] =
        {
            .name = "Write Standard",
            .key = picopass_iclass_key,
            .is_elite = false,
        },
    [SubmenuIndexWriteiCE] =
        {
            .name = "Write iCE",
            .key = picopass_xice_key,
            .is_elite = true,
        },
    [SubmenuIndexWriteiCL] =
        {
            .name = "Write iCL",
            .key = picopass_xicl_key,
            .is_elite = false,
        },
    [SubmenuIndexWriteiCS] =
        {
            .name = "Write iCS",
            .key = picopass_xics_key,
            .is_elite = false,
        },
};

void picopass_scene_key_menu_submenu_callback(void* context, uint32_t index) {
    Picopass* picopass = context;

    view_dispatcher_send_custom_event(picopass->view_dispatcher, index);
}

void picopass_scene_key_menu_on_enter(void* context) {
    Picopass* picopass = context;
    Submenu* submenu = picopass->submenu;
    memset(&picopass->write_key_context, 0, sizeof(PicopassWriteKeyContext));

    for(size_t i = 0; i < SubmenuIndexWriteCustom; i++) {
        submenu_add_item(
            submenu,
            picopass_scene_key_data[i].name,
            i,
            picopass_scene_key_menu_submenu_callback,
            picopass);
    }
    submenu_add_item(
        submenu,
        "Write Elite",
        SubmenuIndexWriteCustom,
        picopass_scene_key_menu_submenu_callback,
        picopass);

    submenu_set_selected_item(
        picopass->submenu,
        scene_manager_get_scene_state(picopass->scene_manager, PicopassSceneKeyMenu));

    view_dispatcher_switch_to_view(picopass->view_dispatcher, PicopassViewMenu);
}

bool picopass_scene_key_menu_on_event(void* context, SceneManagerEvent event) {
    Picopass* picopass = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event < SubmenuIndexWriteCustom) {
            memcpy(
                picopass->write_key_context.key_to_write,
                picopass_scene_key_data[event.event].key,
                PICOPASS_KEY_LEN);
            picopass->write_key_context.is_elite = picopass_scene_key_data[event.event].is_elite;
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneWriteKey);
            consumed = true;
        } else if(event.event == SubmenuIndexWriteCustom) {
            // If user dictionary, prepopulate with the first key
            if(nfc_dict_check_presence(PICOPASS_ICLASS_ELITE_DICT_USER_NAME)) {
                picopass->dict = nfc_dict_alloc(
                    PICOPASS_ICLASS_ELITE_DICT_USER_NAME,
                    NfcDictModeOpenExisting,
                    PICOPASS_KEY_LEN);
                nfc_dict_get_next_key(
                    picopass->dict, picopass->byte_input_store, PICOPASS_KEY_LEN);
                nfc_dict_free(picopass->dict);
            }
            // Key and elite_kdf = true are both set in key_input scene after the value is input
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneKeyInput);
            consumed = true;
        }
        scene_manager_set_scene_state(picopass->scene_manager, PicopassSceneKeyMenu, event.event);
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            picopass->scene_manager, PicopassSceneStart);
    }

    return consumed;
}

void picopass_scene_key_menu_on_exit(void* context) {
    Picopass* picopass = context;

    submenu_reset(picopass->submenu);
}
