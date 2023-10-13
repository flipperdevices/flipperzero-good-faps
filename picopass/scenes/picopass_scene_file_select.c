#include "../picopass_i.h"
#include "../picopass_device.h"

void picopass_scene_file_select_on_enter(void* context) {
    Picopass* instance = context;

    if(picopass_load_from_file_select(instance)) {
        scene_manager_next_scene(instance->scene_manager, PicopassSceneSavedMenu);
    } else {
        scene_manager_previous_scene(instance->scene_manager);
    }
}

bool picopass_scene_file_select_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void picopass_scene_file_select_on_exit(void* context) {
    UNUSED(context);
}
