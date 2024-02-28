#include <storage/storage.h>
#include "game_settings.h"
#include <lib/toolbox/saved_struct.h>

#define SETTINGS_PATH APP_DATA_PATH("settings.bin")
#define SETTINGS_VERSION (0)
#define SETTINGS_MAGIC (0x69)

#define STATE_PATH APP_DATA_PATH("state.bin")
#define STATE_VERSION (2)
#define STATE_MAGIC (0x88)

bool game_settings_load(Settings* settings) {
    furi_assert(settings);
    return saved_struct_load(
        SETTINGS_PATH, settings, sizeof(Settings), SETTINGS_MAGIC, SETTINGS_VERSION);
}

bool game_settings_save(Settings* settings) {
    furi_assert(settings);
    return saved_struct_save(
        SETTINGS_PATH, settings, sizeof(Settings), SETTINGS_MAGIC, SETTINGS_VERSION);
}

bool game_state_load(SaveState* save_state) {
    furi_assert(save_state);
    return saved_struct_load(
        STATE_PATH, save_state, sizeof(SaveState), STATE_MAGIC, STATE_VERSION);
}

bool game_state_save(SaveState* save_state) {
    furi_assert(save_state);
    return saved_struct_save(
        STATE_PATH, save_state, sizeof(SaveState), STATE_MAGIC, STATE_VERSION);
}

void game_state_reset(SaveState* save_state) {
    furi_assert(save_state);
    memset(save_state, 0, sizeof(SaveState));
    save_state->lives = 2;
    game_state_save(save_state);
}