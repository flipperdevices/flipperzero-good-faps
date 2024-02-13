#include "../nfc_magic_app_i.h"

void nfc_magic_scene_mf_classic_write_check_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    NfcMagicApp* instance = context;

    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, result);
    }
}

void nfc_magic_scene_mf_classic_write_check_on_enter(void* context) {
    NfcMagicApp* instance = context;
    Widget* widget = instance->widget;
    bool can_write_all = gen2_poller_can_write_everything(instance->source_dev);

    if(can_write_all) {
        scene_manager_next_scene(instance->scene_manager, NfcMagicSceneWrite);
    }
    widget_add_string_element(
        widget, 3, 0, AlignLeft, AlignTop, FontPrimary, "Not all can be written");
    widget_add_text_box_element(
        widget, 0, 13, 128, 54, AlignLeft, AlignTop, "Something something access bits", false);
    widget_add_button_element(
        widget,
        GuiButtonTypeCenter,
        "Continue",
        nfc_magic_scene_mf_classic_write_check_widget_callback,
        instance);
    widget_add_button_element(
        widget,
        GuiButtonTypeLeft,
        "Back",
        nfc_magic_scene_mf_classic_write_check_widget_callback,
        instance);

    // Setup and start worker
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewWidget);
}

bool nfc_magic_scene_mf_classic_write_check_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_previous_scene(instance->scene_manager);
        } else if(event.event == GuiButtonTypeCenter) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneWrite);
            consumed = true;
        }
    }
    return consumed;
}

void nfc_magic_scene_mf_classic_write_check_on_exit(void* context) {
    NfcMagicApp* instance = context;

    widget_reset(instance->widget);
}
