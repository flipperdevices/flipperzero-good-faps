#include "../flip_tdi_app_i.h"

void flip_tdi_scene_close_rpc_on_enter(void* context) {
    FlipTDIApp* app = context;

    widget_add_icon_element(app->widget, 78, 0, &I_ActiveConnection_50x64);
    widget_add_string_multiline_element(
        app->widget, 3, 2, AlignLeft, AlignTop, FontPrimary, "Connection\nIs Active!");
    widget_add_string_multiline_element(
        app->widget,
        3,
        30,
        AlignLeft,
        AlignTop,
        FontSecondary,
        "Disconnect from\nPC or phone to\nuse this function.");

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipTDIViewWidget);
}

bool flip_tdi_scene_close_rpc_on_event(void* context, SceneManagerEvent event) {
    FlipTDIApp* app = context;

    if(event.type == SceneManagerEventTypeBack) {
        view_dispatcher_stop(app->view_dispatcher);
        return true;
    }

    return false;
}

void flip_tdi_scene_close_rpc_on_exit(void* context) {
    FlipTDIApp* app = context;
    widget_reset(app->widget);
}
