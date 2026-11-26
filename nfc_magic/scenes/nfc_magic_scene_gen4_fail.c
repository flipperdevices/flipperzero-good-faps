#include "../nfc_magic_app_i.h"

static const char* nfc_magic_scene_gen4_fail_get_message(Gen4PollerError error) {
    switch(error) {
    case Gen4PollerErrorWrongProtocol:
        return "Card is not in the\nrequired protocol.\nNothing was written.";
    case Gen4PollerErrorInvalidParams:
        return "Refused to apply.\nCard was not modified.";
    case Gen4PollerErrorTimeout:
        return "Card lost or not\nresponding. Card may\nbe partially written.";
    case Gen4PollerErrorProtocol:
        return "Write failed. Card\nmay be partially\nwritten.";
    default:
        return "No response. Maybe\nnot Gen4 card?";
    }
}

void nfc_magic_scene_gen4_fail_widget_callback(GuiButtonType result, InputType type, void* context) {
    NfcMagicApp* instance = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, result);
    }
}

void nfc_magic_scene_gen4_fail_on_enter(void* context) {
    NfcMagicApp* instance = context;
    Widget* widget = instance->widget;

    notification_message(instance->notifications, &sequence_error);

    widget_add_icon_element(widget, 72, 17, &I_DolphinCommon_56x48);
    widget_add_string_element(
        widget, 7, 4, AlignLeft, AlignTop, FontPrimary, "Something gone wrong!");
    widget_add_string_multiline_element(
        widget,
        7,
        17,
        AlignLeft,
        AlignTop,
        FontSecondary,
        nfc_magic_scene_gen4_fail_get_message(instance->gen4_last_error));

    widget_add_button_element(
        widget, GuiButtonTypeLeft, "Back", nfc_magic_scene_gen4_fail_widget_callback, instance);

    // Setup and start worker
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewWidget);
}

bool nfc_magic_scene_gen4_fail_on_event(void* context, SceneManagerEvent event) {
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

void nfc_magic_scene_gen4_fail_on_exit(void* context) {
    NfcMagicApp* instance = context;

    widget_reset(instance->widget);
}
