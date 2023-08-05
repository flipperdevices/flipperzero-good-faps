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

Picopass* picopass_alloc() {
    Picopass* picopass = malloc(sizeof(Picopass));

    picopass->worker = picopass_worker_alloc();
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

    // Picopass device
    picopass->dev = picopass_device_alloc();

    // Open GUI record
    picopass->gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(
        picopass->view_dispatcher, picopass->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    picopass->notifications = furi_record_open(RECORD_NOTIFICATION);

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

    return picopass;
}

void picopass_free(Picopass* picopass) {
    furi_assert(picopass);

    // Picopass device
    picopass_device_free(picopass->dev);
    picopass->dev = NULL;

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

    // Worker
    picopass_worker_stop(picopass->worker);
    picopass_worker_free(picopass->worker);

    // View Dispatcher
    view_dispatcher_free(picopass->view_dispatcher);

    // Scene Manager
    scene_manager_free(picopass->scene_manager);

    // GUI
    furi_record_close(RECORD_GUI);
    picopass->gui = NULL;

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    picopass->notifications = NULL;

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

void picopass_show_loading_popup(void* context, bool show) {
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

int32_t picopass_app(void* p) {
    UNUSED(p);
    picopass_migrate_from_old_folder();

    Picopass* picopass = picopass_alloc();

    scene_manager_next_scene(picopass->scene_manager, PicopassSceneStart);

    view_dispatcher_run(picopass->view_dispatcher);

    picopass_free(picopass);

    return 0;
}
