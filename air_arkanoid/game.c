#include "game.h"
#include "game_settings.h"
#include "levels/level_menu.h"
#include "levels/level_game.h"
#include "levels/level_settings.h"
#include "levels/level_message.h"
#include "levels/level_pause.h"

const NotificationSequence sequence_sound_blip = {
    &message_note_c7,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

const NotificationSequence sequence_sound_menu = {
    &message_note_c6,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

void game_start(GameManager* game_manager, void* ctx) {
    GameContext* context = ctx;
    context->imu = imu_alloc();
    context->imu_present = imu_present(context->imu);
    context->levels.menu = game_manager_add_level(game_manager, &level_menu);
    context->levels.settings = game_manager_add_level(game_manager, &level_settings);
    context->levels.game = game_manager_add_level(game_manager, &level_game);
    context->levels.message = game_manager_add_level(game_manager, &level_message);
    context->levels.pause = game_manager_add_level(game_manager, &level_pause);

    if(!game_settings_load(&context->settings)) {
        context->settings.sound = true;
        context->settings.show_fps = false;
    }

    if(!game_state_load(&context->save_state)) {
        game_state_reset(&context->save_state);
    }

    context->state = context->save_state;

    context->app = furi_record_open(RECORD_NOTIFICATION);
    context->game_manager = game_manager;

    game_manager_show_fps_set(context->game_manager, context->settings.show_fps);
}

void game_stop(void* ctx) {
    GameContext* context = ctx;
    imu_free(context->imu);

    furi_record_close(RECORD_NOTIFICATION);
}

const Game game = {
    .target_fps = 30,
    .show_fps = false,
    .always_backlight = true,
    .start = game_start,
    .stop = game_stop,
    .context_size = sizeof(GameContext),
};

void game_switch_sound(GameContext* context) {
    context->settings.sound = !context->settings.sound;
    game_settings_save(&context->settings);
}

void game_switch_show_fps(GameContext* context) {
    context->settings.show_fps = !context->settings.show_fps;
    game_manager_show_fps_set(context->game_manager, context->settings.show_fps);
    game_settings_save(&context->settings);
}

void game_sound_play(GameContext* context, const NotificationSequence* sequence) {
    if(context->settings.sound) {
        notification_message(context->app, sequence);
    }
}

#include <storage/storage.h>

static const char* game_levels[] = {
    APP_ASSETS_PATH("levels/1.flaam"),
    APP_ASSETS_PATH("levels/2.flaam"),
    APP_ASSETS_PATH("levels/3.flaam"),
};

const size_t level_count = sizeof(game_levels) / sizeof(game_levels[0]);

const char* game_level_get(GameContext* context) {
    if(context->state.level_index < level_count) {
        return game_levels[context->state.level_index];
    } else {
        return NULL;
    }
}

static void game_level_clear_cb(Level* level, void* ctx) {
    GameContext* context = ctx;
    level_load(level, game_level_get(context));
    game_sound_play(context, &sequence_level_start);
}

void game_level_reset(GameContext* context) {
    context->state = context->save_state;
    level_clear(context->levels.game, game_level_clear_cb, context);
}

bool game_level_is_last(GameContext* context) {
    return context->state.level_index == level_count - 1;
}

void game_level_next(GameContext* context) {
    context->state.level_index++;
    context->save_state = context->state;
    game_state_save(&context->state);
}

void game_level_new_game_plus(GameContext* context) {
    context->state.level_index = 0;
    context->state.new_game_index++;
    context->save_state = context->state;
    game_state_save(&context->state);
}