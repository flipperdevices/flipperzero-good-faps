#include "picopass_i.h"

#define TAG "PicoPass"

bool picopass_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Picopass* picopass = context;
    return scene_manager_handle_custom_event(picopass->scene_manager, event);
}

bool picopass_back_event_callback(void* context) {
    furi_assert(context);
    Picopass* picopass = context;
    return scene_manager_handle_back_event(picopass->scene_manager);
}

void picopass_tick_event_callback(void* context) {
    furi_assert(context);
    Picopass* picopass = context;
    scene_manager_handle_tick_event(picopass->scene_manager);
}

static void picopass_show_loading_popup(void* context, bool show) {
    Picopass* picopass = context;
    TaskHandle_t timer_task = xTaskGetHandle(configTIMER_SERVICE_TASK_NAME);

    if(show) {
        // Raise timer priority so that animations can play
        vTaskPrioritySet(timer_task, configMAX_PRIORITIES - 1);
        view_dispatcher_switch_to_view(picopass->view_dispatcher, PicopassViewLoading);
    } else {
        // Restore default timer priority
        vTaskPrioritySet(timer_task, configTIMER_TASK_PRIORITY);
    }
}

Picopass* picopass_alloc() {
    Picopass* picopass = malloc(sizeof(Picopass));

    picopass->view_dispatcher = view_dispatcher_alloc();
    picopass->scene_manager = scene_manager_alloc(&picopass_scene_handlers, picopass);
    view_dispatcher_enable_queue(picopass->view_dispatcher);
    view_dispatcher_set_event_callback_context(picopass->view_dispatcher, picopass);
    view_dispatcher_set_custom_event_callback(
        picopass->view_dispatcher, picopass_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        picopass->view_dispatcher, picopass_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        picopass->view_dispatcher, picopass_tick_event_callback, 100);

    picopass->nfc = nfc_alloc();

    // Picopass device
    picopass->device = picopass_dev_alloc();
    picopass_dev_set_loading_callback(picopass->device, picopass_show_loading_popup, picopass);

    // Open GUI record
    picopass->gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(
        picopass->view_dispatcher, picopass->gui, ViewDispatcherTypeFullscreen);

    // Open Storage record
    picopass->storage = furi_record_open(RECORD_STORAGE);

    // Open Notification record
    picopass->notifications = furi_record_open(RECORD_NOTIFICATION);

    // Open Dialogs record
    picopass->dialogs = furi_record_open(RECORD_DIALOGS);

    // Submenu
    picopass->submenu = submenu_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher, PicopassViewMenu, submenu_get_view(picopass->submenu));

    // Popup
    picopass->popup = popup_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher, PicopassViewPopup, popup_get_view(picopass->popup));

    // Loading
    picopass->loading = loading_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher, PicopassViewLoading, loading_get_view(picopass->loading));

    // Text Input
    picopass->text_input = text_input_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher,
        PicopassViewTextInput,
        text_input_get_view(picopass->text_input));

    // Byte Input
    picopass->byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher,
        PicopassViewByteInput,
        byte_input_get_view(picopass->byte_input));

    // Custom Widget
    picopass->widget = widget_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher, PicopassViewWidget, widget_get_view(picopass->widget));

    picopass->dict_attack = dict_attack_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher,
        PicopassViewDictAttack,
        dict_attack_get_view(picopass->dict_attack));

    picopass->loclass = loclass_alloc();
    view_dispatcher_add_view(
        picopass->view_dispatcher, PicopassViewLoclass, loclass_get_view(picopass->loclass));

    picopass->file_path = furi_string_alloc_set_str(STORAGE_APP_DATA_PATH_PREFIX);
    picopass->file_name = furi_string_alloc();

    return picopass;
}

void picopass_free(Picopass* picopass) {
    furi_assert(picopass);

    // Picopass device
    picopass_dev_free(picopass->device);
    furi_string_free(picopass->file_path);
    furi_string_free(picopass->file_name);

    nfc_free(picopass->nfc);

    // Submenu
    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewMenu);
    submenu_free(picopass->submenu);

    // Popup
    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewPopup);
    popup_free(picopass->popup);

    // Loading
    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewLoading);
    loading_free(picopass->loading);

    // TextInput
    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewTextInput);
    text_input_free(picopass->text_input);

    // ByteInput
    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewByteInput);
    byte_input_free(picopass->byte_input);

    // Custom Widget
    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewWidget);
    widget_free(picopass->widget);

    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewDictAttack);
    dict_attack_free(picopass->dict_attack);

    view_dispatcher_remove_view(picopass->view_dispatcher, PicopassViewLoclass);
    loclass_free(picopass->loclass);

    // View Dispatcher
    view_dispatcher_free(picopass->view_dispatcher);

    // Scene Manager
    scene_manager_free(picopass->scene_manager);

    // Storage
    furi_record_close(RECORD_STORAGE);

    // GUI
    furi_record_close(RECORD_GUI);

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);

    // Dialogs
    furi_record_close(RECORD_DIALOGS);

    free(picopass);
}

void picopass_text_store_set(Picopass* picopass, const char* text, ...) {
    va_list args;
    va_start(args, text);

    vsnprintf(picopass->text_store, sizeof(picopass->text_store), text, args);

    va_end(args);
}

void picopass_text_store_clear(Picopass* picopass) {
    memset(picopass->text_store, 0, sizeof(picopass->text_store));
}

static const NotificationSequence picopass_sequence_blink_start_cyan = {
    &message_blink_start_10,
    &message_blink_set_color_cyan,
    &message_do_not_reset,
    NULL,
};

static const NotificationSequence picopass_sequence_blink_start_magenta = {
    &message_blink_start_10,
    &message_blink_set_color_magenta,
    &message_do_not_reset,
    NULL,
};

static const NotificationSequence picopass_sequence_blink_stop = {
    &message_blink_stop,
    NULL,
};

void picopass_blink_start(Picopass* picopass) {
    notification_message(picopass->notifications, &picopass_sequence_blink_start_cyan);
}

void picopass_blink_emulate_start(Picopass* picopass) {
    notification_message(picopass->notifications, &picopass_sequence_blink_start_magenta);
}

void picopass_blink_stop(Picopass* picopass) {
    notification_message(picopass->notifications, &picopass_sequence_blink_stop);
}

static void picopass_migrate_from_old_folder() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_migrate(storage, "/ext/picopass", STORAGE_APP_DATA_PATH_PREFIX);
    furi_record_close(RECORD_STORAGE);
}

bool picopass_is_memset(const uint8_t* data, const uint8_t pattern, size_t size) {
    bool result = size > 0;
    while(size > 0) {
        result &= (*data == pattern);
        data++;
        size--;
    }
    return result;
}

bool picopass_save(Picopass* instance) {
    furi_assert(instance);
    bool result = false;

    picopass_make_app_folder(instance);

    if(furi_string_end_with(instance->file_path, PICOPASS_APP_EXTENSION)) {
        size_t filename_start = furi_string_search_rchar(instance->file_path, '/');
        furi_string_left(instance->file_path, filename_start);
    }

    furi_string_cat_printf(
        instance->file_path,
        "/%s%s",
        furi_string_get_cstr(instance->file_name),
        PICOPASS_APP_EXTENSION);

    result = picopass_dev_save(instance->device, furi_string_get_cstr(instance->file_path));

    if(!result) {
        dialog_message_show_storage_error(instance->dialogs, "Cannot save\nkey file");
    }

    return result;
}

bool picopass_save_as_lfrfid(Picopass* instance) {
    furi_assert(instance);
    bool result = false;

    FuriString* lfrfid_file_path = furi_string_alloc_set_str(LFRFID_APP_FOLDER);

    furi_string_cat_printf(
        lfrfid_file_path, "/%s%s", furi_string_get_cstr(instance->file_name), LFRFID_APP_EXTENSION);

    result = picopass_dev_save_as_lfrfid(instance->device, furi_string_get_cstr(lfrfid_file_path));

    if(!result) {
        dialog_message_show_storage_error(instance->dialogs, "Cannot save\nkey file");
    }

    furi_string_free(lfrfid_file_path);

    return result;
}

bool picopass_delete(Picopass* instance) {
    furi_assert(instance);

    return storage_simply_remove(instance->storage, furi_string_get_cstr(instance->file_path));
}

bool picopass_load_from_file_select(Picopass* instance) {
    furi_assert(instance);

    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, PICOPASS_APP_EXTENSION, &I_Nfc_10px);
    browser_options.base_path = STORAGE_APP_DATA_PATH_PREFIX;
    browser_options.hide_dot_files = true;

    // Input events and views are managed by file_browser
    bool result = dialog_file_browser_show(
        instance->dialogs, instance->file_path, instance->file_path, &browser_options);

    if(result) {
        result = picopass_load_file(instance, instance->file_path, true);
    }

    return result;
}

bool picopass_load_file(Picopass* instance, FuriString* path, bool show_dialog) {
    furi_assert(instance);

    bool result = false;

    FuriString* load_path = furi_string_alloc();
    furi_string_set(load_path, path);

    result = picopass_dev_load(instance->device, furi_string_get_cstr(load_path));

    if(result) {
        path_extract_filename(load_path, instance->file_name, true);
    }

    if((!result) && (show_dialog)) {
        dialog_message_show_storage_error(instance->dialogs, "Cannot load\nkey file");
    }

    furi_string_free(load_path);

    return result;
}

void picopass_make_app_folder(Picopass* instance) {
    furi_assert(instance);

    if(!storage_simply_mkdir(instance->storage, STORAGE_APP_DATA_PATH_PREFIX)) {
        dialog_message_show_storage_error(instance->dialogs, "Cannot create\napp folder");
    }
}

int32_t picopass_app(void* p) {
    UNUSED(p);
    picopass_migrate_from_old_folder();

    Picopass* picopass = picopass_alloc();

    scene_manager_next_scene(picopass->scene_manager, PicopassSceneStart);

    view_dispatcher_run(picopass->view_dispatcher);

    picopass_free(picopass);

    return 0;
}
