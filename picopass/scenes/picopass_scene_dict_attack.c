#include "../picopass_i.h"
#include <dolphin/dolphin.h>
#include "../picopass_keys.h"

#define PICOPASS_SCENE_DICT_ATTACK_KEYS_BATCH_UPDATE (10)

enum {
    PicopassSceneDictAttackDictEliteUser,
    PicopassSceneDictAttackDictStandart,
    PicopassSceneDictAttackDictElite,
};

const char* picopass_dict_name[] = {
    [PicopassSceneDictAttackDictEliteUser] = "Elite User Dictionary",
    [PicopassSceneDictAttackDictStandart] = "Standard System Dictionary",
    [PicopassSceneDictAttackDictElite] = "Elite System Dictionary",
};

static bool picopass_dict_attack_change_dict(Picopass* picopass) {
    bool success = false;

    do {
        uint32_t scene_state =
            scene_manager_get_scene_state(picopass->scene_manager, PicopassSceneDictAttack);
        nfc_dict_free(picopass->dict);
        picopass->dict = NULL;
        if(scene_state == PicopassSceneDictAttackDictElite) break;
        if(scene_state == PicopassSceneDictAttackDictEliteUser) {
            if(!nfc_dict_check_presence(PICOPASS_ICLASS_STANDARD_DICT_FLIPPER_NAME)) break;
            picopass->dict = nfc_dict_alloc(
                PICOPASS_ICLASS_STANDARD_DICT_FLIPPER_NAME,
                NfcDictModeOpenExisting,
                PICOPASS_KEY_LEN);
            scene_state = PicopassSceneDictAttackDictStandart;
        } else if(scene_state == PicopassSceneDictAttackDictStandart) {
            if(!nfc_dict_check_presence(PICOPASS_ICLASS_ELITE_DICT_FLIPPER_NAME)) break;
            picopass->dict = nfc_dict_alloc(
                PICOPASS_ICLASS_ELITE_DICT_FLIPPER_NAME,
                NfcDictModeOpenExisting,
                PICOPASS_KEY_LEN);
            scene_state = PicopassSceneDictAttackDictElite;
        }
        picopass->dict_attack_ctx.card_detected = true;
        picopass->dict_attack_ctx.total_keys = nfc_dict_get_total_keys(picopass->dict);
        picopass->dict_attack_ctx.current_key = 0;
        picopass->dict_attack_ctx.name = picopass_dict_name[scene_state];
        scene_manager_set_scene_state(
            picopass->scene_manager, PicopassSceneDictAttack, scene_state);
        success = true;
    } while(false);

    return success;
}

NfcCommand picopass_dict_attack_worker_callback(PicopassPollerEvent event, void* context) {
    furi_assert(context);
    NfcCommand command = NfcCommandContinue;

    Picopass* picopass = context;

    if(event.type == PicopassPollerEventTypeRequestMode) {
        event.data->req_mode.mode = PicopassPollerModeRead;
    } else if(event.type == PicopassPollerEventTypeRequestKey) {
        uint8_t key[PICOPASS_KEY_LEN] = {};
        bool is_key_provided = true;
        if(!nfc_dict_get_next_key(picopass->dict, key, PICOPASS_KEY_LEN)) {
            if(picopass_dict_attack_change_dict(picopass)) {
                is_key_provided = nfc_dict_get_next_key(picopass->dict, key, PICOPASS_KEY_LEN);
                view_dispatcher_send_custom_event(
                    picopass->view_dispatcher, PicopassCustomEventDictAttackUpdateView);
            } else {
                is_key_provided = false;
            }
        }
        uint32_t scene_state =
            scene_manager_get_scene_state(picopass->scene_manager, PicopassSceneDictAttack);
        memcpy(event.data->req_key.key, key, PICOPASS_KEY_LEN);
        event.data->req_key.is_elite_key = (scene_state != PicopassSceneDictAttackDictStandart);
        event.data->req_key.is_key_provided = is_key_provided;
        if(is_key_provided) {
            picopass->dict_attack_ctx.current_key++;
            if(picopass->dict_attack_ctx.current_key %
                   PICOPASS_SCENE_DICT_ATTACK_KEYS_BATCH_UPDATE ==
               0) {
                view_dispatcher_send_custom_event(
                    picopass->view_dispatcher, PicopassCustomEventDictAttackUpdateView);
            }
        }
    } else if(event.type == PicopassPollerEventTypeSuccess) {
        const PicopassData* data = picopass_poller_get_data(picopass->poller);
        picopass_dev_set_data(picopass->device, data);
        view_dispatcher_send_custom_event(
            picopass->view_dispatcher, PicopassCustomEventPollerSuccess);
    } else if(event.type == PicopassPollerEventTypeFail) {
        const PicopassData* data = picopass_poller_get_data(picopass->poller);
        picopass_dev_set_data(picopass->device, data);
        view_dispatcher_send_custom_event(
            picopass->view_dispatcher, PicopassCustomEventPollerSuccess);
    } else if(event.type == PicopassPollerEventTypeCardLost) {
        picopass->dict_attack_ctx.card_detected = false;
        view_dispatcher_send_custom_event(
            picopass->view_dispatcher, PicopassCustomEventDictAttackUpdateView);
    } else if(event.type == PicopassPollerEventTypeCardDetected) {
        picopass->dict_attack_ctx.card_detected = true;
        view_dispatcher_send_custom_event(
            picopass->view_dispatcher, PicopassCustomEventDictAttackUpdateView);
    }

    return command;
}

static void picopass_scene_dict_attack_update_view(Picopass* instance) {
    if(instance->dict_attack_ctx.card_detected) {
        dict_attack_set_card_detected(instance->dict_attack);
        dict_attack_set_header(instance->dict_attack, instance->dict_attack_ctx.name);
        dict_attack_set_total_dict_keys(
            instance->dict_attack, instance->dict_attack_ctx.total_keys);
        dict_attack_set_current_dict_key(
            instance->dict_attack, instance->dict_attack_ctx.current_key);
    } else {
        dict_attack_set_card_removed(instance->dict_attack);
    }
}

static void picopass_scene_dict_attack_callback(void* context) {
    Picopass* instance = context;

    view_dispatcher_send_custom_event(
        instance->view_dispatcher, PicopassCustomEventDictAttackSkip);
}

void picopass_scene_dict_attack_on_enter(void* context) {
    Picopass* picopass = context;
    dolphin_deed(DolphinDeedNfcRead);

    // Setup dict attack context
    uint32_t state = PicopassSceneDictAttackDictEliteUser;

    bool use_user_dict = nfc_dict_check_presence(PICOPASS_ICLASS_ELITE_DICT_USER_NAME);
    if(use_user_dict) {
        picopass->dict = nfc_dict_alloc(
            PICOPASS_ICLASS_ELITE_DICT_USER_NAME, NfcDictModeOpenExisting, PICOPASS_KEY_LEN);
        if(nfc_dict_get_total_keys(picopass->dict) == 0) {
            nfc_dict_free(picopass->dict);
            use_user_dict = false;
        }
    }
    if(use_user_dict) {
        state = PicopassSceneDictAttackDictEliteUser;
    } else {
        picopass->dict = nfc_dict_alloc(
            PICOPASS_ICLASS_STANDARD_DICT_FLIPPER_NAME, NfcDictModeOpenExisting, PICOPASS_KEY_LEN);
        state = PicopassSceneDictAttackDictStandart;
    }
    picopass->dict_attack_ctx.card_detected = true;
    picopass->dict_attack_ctx.total_keys = nfc_dict_get_total_keys(picopass->dict);
    picopass->dict_attack_ctx.current_key = 0;
    picopass->dict_attack_ctx.name = picopass_dict_name[state];
    scene_manager_set_scene_state(picopass->scene_manager, PicopassSceneDictAttack, state);

    // Setup view
    picopass_scene_dict_attack_update_view(picopass);
    dict_attack_set_callback(picopass->dict_attack, picopass_scene_dict_attack_callback, picopass);

    // Start worker
    picopass->poller = picopass_poller_alloc(picopass->nfc);
    picopass_poller_start(picopass->poller, picopass_dict_attack_worker_callback, picopass);

    view_dispatcher_switch_to_view(picopass->view_dispatcher, PicopassViewDictAttack);
    picopass_blink_start(picopass);
}

bool picopass_scene_dict_attack_on_event(void* context, SceneManagerEvent event) {
    Picopass* picopass = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == PicopassCustomEventPollerSuccess) {
            const PicopassData* data = picopass_dev_get_data(picopass->device);
            if(memcmp(data->pacs.key, picopass_factory_debit_key, PICOPASS_BLOCK_LEN) == 0) {
                scene_manager_next_scene(picopass->scene_manager, PicopassSceneReadFactorySuccess);
            } else {
                scene_manager_next_scene(picopass->scene_manager, PicopassSceneReadCardSuccess);
            }
            consumed = true;
        } else if(event.event == PicopassCustomEventDictAttackUpdateView) {
            picopass_scene_dict_attack_update_view(picopass);
            consumed = true;
        } else if(event.event == PicopassCustomEventDictAttackSkip) {
            uint32_t scene_state =
                scene_manager_get_scene_state(picopass->scene_manager, PicopassSceneDictAttack);
            if(scene_state != PicopassSceneDictAttackDictElite) {
                picopass_dict_attack_change_dict(picopass);
                picopass_scene_dict_attack_update_view(picopass);
            } else {
                const PicopassData* data = picopass_dev_get_data(picopass->device);
                if(memcmp(data->pacs.key, picopass_factory_debit_key, PICOPASS_BLOCK_LEN) == 0) {
                    scene_manager_next_scene(
                        picopass->scene_manager, PicopassSceneReadFactorySuccess);
                } else {
                    scene_manager_next_scene(
                        picopass->scene_manager, PicopassSceneReadCardSuccess);
                }
            }
            consumed = true;
        }
    }
    return consumed;
}

void picopass_scene_dict_attack_on_exit(void* context) {
    Picopass* picopass = context;

    if(picopass->dict) {
        nfc_dict_free(picopass->dict);
        picopass->dict = NULL;
    }
    picopass->dict_attack_ctx.current_key = 0;
    picopass->dict_attack_ctx.total_keys = 0;

    picopass_poller_stop(picopass->poller);
    picopass_poller_free(picopass->poller);

    // Clear view
    popup_reset(picopass->popup);
    scene_manager_set_scene_state(
        picopass->scene_manager, PicopassSceneDictAttack, PicopassSceneDictAttackDictEliteUser);

    picopass_blink_stop(picopass);
}
