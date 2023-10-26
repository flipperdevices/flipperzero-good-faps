#include "../nfc_magic_app_i.h"

void nfc_magic_scene_wrong_card_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    NfcMagicApp* instance = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, result);
    }
}

void nfc_magic_scene_wrong_card_on_enter(void* context) {
    NfcMagicApp* instance = context;
    Widget* widget = instance->widget;

    notification_message(instance->notifications, &sequence_error);

    widget_add_icon_element(widget, 73, 17, &I_DolphinCommon_56x48);
    widget_add_string_element(
        widget, 1, 4, AlignLeft, AlignTop, FontPrimary, "This is wrong card");
    widget_add_string_multiline_element(
        widget,
        1,
        17,
        AlignLeft,
        AlignTop,
        FontSecondary,
        "Writing this file is\nnot supported for\nthis magic card.");
    widget_add_button_element(
        widget, GuiButtonTypeLeft, "Retry", nfc_magic_scene_wrong_card_widget_callback, instance);

    // Setup and start worker
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewWidget);
}

bool nfc_magic_scene_wrong_card_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_previous_scene(instance->scene_manager);
        }
    }
    return consumed;
}

void nfc_magic_scene_wrong_card_on_exit(void* context) {
    NfcMagicApp* instance = context;

    widget_reset(instance->widget);
}
