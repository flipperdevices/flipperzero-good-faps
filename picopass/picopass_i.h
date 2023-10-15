#pragma once

#include "picopass.h"
#include "picopass_worker.h"
#include "picopass_device.h"

#include "rfal_picopass.h"

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <notification/notification_messages.h>

#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/loading.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/widget.h>

#include <input/input.h>

#include "scenes/picopass_scene.h"
#include "views/dict_attack.h"
#include "views/loclass.h"

#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <lib/toolbox/path.h>
#include <picopass_icons.h>

#include <nfc/nfc.h>
#include <nfc/helpers/nfc_dict.h>
#include "protocol/picopass_poller.h"
#include "protocol/picopass_listener.h"
#include "picopass_dev.h"

#define PICOPASS_TEXT_STORE_SIZE 128

#define PICOPASS_APP_EXTENSION ".picopass"
#define PICOPASS_APP_FILE_PREFIX "Picopass"

#define PICOPASS_ICLASS_ELITE_DICT_FLIPPER_NAME APP_ASSETS_PATH("iclass_elite_dict.txt")
#define PICOPASS_ICLASS_STANDARD_DICT_FLIPPER_NAME APP_ASSETS_PATH("iclass_standard_dict.txt")
#define PICOPASS_ICLASS_ELITE_DICT_USER_NAME APP_DATA_PATH("assets/iclass_elite_dict_user.txt")

enum PicopassCustomEvent {
    // Reserve first 100 events for button types and indexes, starting from 0
    PicopassCustomEventReserved = 100,

    PicopassCustomEventViewExit,
    PicopassCustomEventWorkerExit,
    PicopassCustomEventByteInputDone,
    PicopassCustomEventTextInputDone,
    PicopassCustomEventDictAttackSkip,
    PicopassCustomEventDictAttackUpdateView,

    PicopassCustomEventPollerSuccess,
    PicopassCustomEventPollerFail,
};

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    const char* name;
    uint16_t total_keys;
    uint16_t current_key;
    bool card_detected;
} PicopassDictAttackContext;

typedef struct {
    uint8_t key_to_write[PICOPASS_BLOCK_LEN];
    bool is_elite;
} PicopassWriteKeyContext;

struct Picopass {
    PicopassWorker* worker;
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    Storage* storage;
    NotificationApp* notifications;
    DialogsApp* dialogs;
    SceneManager* scene_manager;
    PicopassDevice* dev;

    Nfc* nfc;
    PicopassPoller* poller;
    PicopassListener* listener;
    NfcDict* dict;

    char text_store[PICOPASS_TEXT_STORE_SIZE + 1];
    FuriString* text_box_store;
    uint8_t byte_input_store[PICOPASS_BLOCK_LEN];

    // Common Views
    Submenu* submenu;
    Popup* popup;
    Loading* loading;
    TextInput* text_input;
    ByteInput* byte_input;
    Widget* widget;
    DictAttack* dict_attack;
    Loclass* loclass;

    PicopassDictAttackContext dict_attack_ctx;
    PicopassWriteKeyContext write_key_context;
    PicopassDev* device;
    FuriString* file_path;
    FuriString* file_name;
};

typedef enum {
    PicopassViewMenu,
    PicopassViewPopup,
    PicopassViewLoading,
    PicopassViewTextInput,
    PicopassViewByteInput,
    PicopassViewWidget,
    PicopassViewDictAttack,
    PicopassViewLoclass,
} PicopassView;

Picopass* picopass_alloc();

void picopass_text_store_set(Picopass* picopass, const char* text, ...);

void picopass_text_store_clear(Picopass* picopass);

void picopass_blink_start(Picopass* picopass);

void picopass_blink_emulate_start(Picopass* picopass);

void picopass_blink_stop(Picopass* picopass);

/** Check if memory is set to pattern
 *
 * @warning    zero size will return false
 *
 * @param[in]  data     Pointer to the byte array
 * @param[in]  pattern  The pattern
 * @param[in]  size     The byte array size
 *
 * @return     True if memory is set to pattern, false otherwise
 */
bool picopass_is_memset(const uint8_t* data, const uint8_t pattern, size_t size);

bool picopass_save(Picopass* instance);

bool picopass_delete(Picopass* instance);

bool picopass_load_from_file_select(Picopass* instance);

bool picopass_load_file(Picopass* instance, FuriString* path, bool show_dialog);

void picopass_make_app_folder(Picopass* instance);
