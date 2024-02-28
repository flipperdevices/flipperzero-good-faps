#pragma once
#include "game.h"

bool game_settings_save(Settings* settings);

bool game_settings_load(Settings* settings);

bool game_state_load(SaveState* save_state);

bool game_state_save(SaveState* save_state);

void game_state_reset(SaveState* save_state);