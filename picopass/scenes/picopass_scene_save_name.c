#include "../picopass_i.h"
#include <lib/toolbox/name_generator.h>
#include <gui/modules/validators.h>
#include <toolbox/path.h>

void picopass_scene_save_name_text_input_callback(void* context) {
    Picopass* picopass = context;

    view_dispatcher_send_custom_event(picopass->view_dispatcher, PicopassCustomEventTextInputDone);
}

void picopass_scene_save_name_on_enter(void* context) {
    Picopass* picopass = context;
    FuriString* folder_path = furi_string_alloc();

    // Setup view
    TextInput* text_input = picopass->text_input;
    bool dev_name_empty = furi_string_empty(picopass->file_name);

    if(dev_name_empty) {
        furi_string_set(picopass->file_path, STORAGE_APP_DATA_PATH_PREFIX);
        name_generator_make_auto(
            picopass->text_store, sizeof(picopass->text_store), PICOPASS_APP_FILE_PREFIX);
        furi_string_set(folder_path, STORAGE_APP_DATA_PATH_PREFIX);
        dev_name_empty = true;
    } else {
        picopass_text_store_set(picopass, "%s", furi_string_get_cstr(picopass->file_name));
        path_extract_dirname(furi_string_get_cstr(picopass->file_path), folder_path);
    }
    text_input_set_header_text(text_input, "Name the card");
    text_input_set_result_callback(
        text_input,
        picopass_scene_save_name_text_input_callback,
        picopass,
        picopass->text_store,
        PICOPASS_NAME_MAX_LEN,
        dev_name_empty);

    ValidatorIsFile* validator_is_file = validator_is_file_alloc_init(
        furi_string_get_cstr(folder_path),
        PICOPASS_APP_EXTENSION,
        furi_string_get_cstr(picopass->file_name));
    text_input_set_validator(text_input, validator_is_file_callback, validator_is_file);

    furi_string_free(folder_path);
    view_dispatcher_switch_to_view(picopass->view_dispatcher, PicopassViewTextInput);
}

bool picopass_scene_save_name_on_event(void* context, SceneManagerEvent event) {
    Picopass* picopass = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == PicopassCustomEventTextInputDone) {
            if(!furi_string_empty(picopass->file_name)) {
                picopass_delete(picopass);
            }
            furi_string_set(picopass->file_name, picopass->text_store);

            bool save_success = false;
            if(picopass->save_format == PicopassSaveFormatLfrfid) {
                save_success = picopass_save_as_lfrfid(picopass);
            } else {
                save_success = picopass_save(picopass);
            }
            if(save_success) {
                scene_manager_next_scene(picopass->scene_manager, PicopassSceneSaveSuccess);
                consumed = true;
            } else {
                consumed = scene_manager_search_and_switch_to_previous_scene(
                    picopass->scene_manager, PicopassSceneStart);
            }
        }
    }
    return consumed;
}

void picopass_scene_save_name_on_exit(void* context) {
    Picopass* picopass = context;

    // Clear view
    void* validator_context = text_input_get_validator_callback_context(picopass->text_input);
    text_input_set_validator(picopass->text_input, NULL, NULL);
    validator_is_file_free(validator_context);

    text_input_reset(picopass->text_input);
}
