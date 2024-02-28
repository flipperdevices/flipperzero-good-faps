#pragma once
#include "engine/engine.h"
#include "engine/sensors/imu.h"
#include "sounds/sounds.h"

typedef struct {
    Level* menu;
    Level* settings;
    Level* game;
    Level* message;
    Level* pause;
} Levels;

typedef struct {
    bool sound;
    bool show_fps;
} Settings;

typedef struct {
    int32_t lives;
    uint32_t score;
    uint32_t level_index;
    uint32_t new_game_index;
} SaveState;

typedef struct {
    Imu* imu;
    bool imu_present;

    Levels levels;
    Settings settings;
    SaveState state;
    SaveState save_state;

    NotificationApp* app;
    GameManager* game_manager;
} GameContext;

void game_switch_sound(GameContext* context);

void game_switch_show_fps(GameContext* context);

void game_sound_play(GameContext* context, const NotificationSequence* sequence);

void game_level_reset(GameContext* context);

bool game_level_is_last(GameContext* context);

void game_level_next(GameContext* context);

void game_level_new_game_plus(GameContext* context);

extern const NotificationSequence sequence_sound_menu;