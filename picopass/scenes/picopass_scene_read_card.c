#include "../picopass_i.h"
#include <dolphin/dolphin.h>
#include "../picopass_keys.h"

NfcCommand picopass_read_card_worker_callback(PicopassPollerEvent event, void* context) {
    furi_assert(context);
    NfcCommand command = NfcCommandContinue;

    Picopass* picopass = context;

    if(event.type == PicopassPollerEventTypeSuccess) {
        view_dispatcher_send_custom_event(
            picopass->view_dispatcher, PicopassCustomEventWorkerExit);
    }

    return command;
}

void picopass_scene_read_card_on_enter(void* context) {
    Picopass* picopass = context;
    dolphin_deed(DolphinDeedNfcRead);

    // Setup view
    Popup* popup = picopass->popup;
    popup_set_header(popup, "Detecting\npicopass\ncard", 68, 30, AlignLeft, AlignTop);
    popup_set_icon(popup, 0, 3, &I_RFIDDolphinReceive_97x61);

    // Start worker
    picopass->poller = picopass_poller_alloc(picopass->nfc);
    picopass_poller_start(picopass->poller, picopass_read_card_worker_callback, picopass);

    view_dispatcher_switch_to_view(picopass->view_dispatcher, PicopassViewPopup);
    picopass_blink_start(picopass);
}

bool picopass_scene_read_card_on_event(void* context, SceneManagerEvent event) {
    Picopass* picopass = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == PicopassCustomEventWorkerExit) {
            if(memcmp(
                   picopass->dev->dev_data.pacs.key,
                   picopass_factory_debit_key,
                   RFAL_PICOPASS_BLOCK_LEN) == 0) {
                scene_manager_next_scene(picopass->scene_manager, PicopassSceneReadFactorySuccess);
            } else {
                scene_manager_next_scene(picopass->scene_manager, PicopassSceneReadCardSuccess);
            }
            consumed = true;
        }
    }
    return consumed;
}

void picopass_scene_read_card_on_exit(void* context) {
    Picopass* picopass = context;

    picopass_poller_stop(picopass->poller);
    picopass_poller_free(picopass->poller);

    // Clear view
    popup_reset(picopass->popup);

    picopass_blink_stop(picopass);
}
