#include "../nfc_magic_app_i.h"

void nfc_magic_scene_write_fail_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    NfcMagicApp* instance = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, result);
    }
}

void nfc_magic_scene_write_fail_on_enter(void* context) {
    NfcMagicApp* instance = context;
    Widget* widget = instance->widget;

    notification_message(instance->notifications, &sequence_error);

    widget_add_icon_element(widget, 72, 17, &I_DolphinCommon_56x48);
    widget_add_string_element(
        widget, 7, 4, AlignLeft, AlignTop, FontPrimary, "Writing gone wrong!");
    widget_add_string_multiline_element(
        widget,
        7,
        17,
        AlignLeft,
        AlignTop,
        FontSecondary,
        "Not all sectors\nwere written\ncorrectly.");

    widget_add_button_element(
        widget, GuiButtonTypeLeft, "Finish", nfc_magic_scene_write_fail_widget_callback, instance);

    // Setup and start worker
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewWidget);
}

bool nfc_magic_scene_write_fail_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_search_and_switch_to_previous_scene(
                instance->scene_manager, NfcMagicSceneStart);
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            instance->scene_manager, NfcMagicSceneStart);
    }
    return consumed;
}

void nfc_magic_scene_write_fail_on_exit(void* context) {
    NfcMagicApp* instance = context;

    widget_reset(instance->widget);
}
