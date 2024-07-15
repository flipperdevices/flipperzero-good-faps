#include "../flip_tdi_app_i.h"
#include "../helpers/flip_tdi_types.h"

void flip_tdi_scene_about_widget_callback(GuiButtonType result, InputType type, void* context) {
    furi_assert(context);

    FlipTDIApp* app = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(app->view_dispatcher, result);
    }
}

void flip_tdi_scene_about_on_enter(void* context) {
    furi_assert(context);

    FlipTDIApp* app = context;
    FuriString* temp_str = furi_string_alloc();
    furi_string_printf(temp_str, "\e#%s\n", "Information");

    furi_string_cat_printf(temp_str, "Version: %s\n", FAP_VERSION);
    furi_string_cat_printf(temp_str, "Developed by: %s\n", FLIP_TDI_DEVELOPED);
    furi_string_cat_printf(temp_str, "Github: %s\n\n", FLIP_TDI_GITHUB);

    furi_string_cat_printf(temp_str, "\e#%s\n", "Description");
    furi_string_cat_printf(temp_str, "Emulator FT232H.\n\n");

    furi_string_cat_printf(temp_str, "\e#%s\n", "What it can do:");
    furi_string_cat_printf(temp_str, "- Emulate FT232H VCP mode\n");
    furi_string_cat_printf(temp_str, "- Emulate FT232H Async bitband mode, max freq 100kHz \n");
    furi_string_cat_printf(temp_str, "- Emulate FT232H Sync bitband mode, max freq 100kHz \n");

    widget_add_text_box_element(
        app->widget,
        0,
        0,
        128,
        14,
        AlignCenter,
        AlignBottom,
        "\e#\e!                                                       \e!\n",
        false);
    widget_add_text_box_element(
        app->widget,
        0,
        2,
        128,
        14,
        AlignCenter,
        AlignBottom,
        "\e#\e!                FlipTDI               \e!\n",
        false);
    widget_add_text_scroll_element(app->widget, 0, 16, 128, 50, furi_string_get_cstr(temp_str));
    furi_string_free(temp_str);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipTDIViewWidget);
}

bool flip_tdi_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void flip_tdi_scene_about_on_exit(void* context) {
    furi_assert(context);

    FlipTDIApp* app = context;
    // Clear views
    widget_reset(app->widget);
}
