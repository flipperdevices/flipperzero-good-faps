#include <gui/elements.h>
#include "level_message.h"

typedef enum {
    MessageStateNoLives,
    MessageStateLevelComplete,
    MessageStateGameComplete,
} MessageState;

MessageState message_state(GameContext* game) {
    if(game->state.lives < 0) {
        return MessageStateNoLives;
    }
    if(game_level_is_last(game)) {
        return MessageStateGameComplete;
    }
    return MessageStateLevelComplete;
}

static void message_update(Entity* self, GameManager* manager, void* context) {
    UNUSED(self);
    UNUSED(context);
    InputState input = game_manager_input_get(manager);

    GameContext* game = game_manager_game_context_get(manager);
    switch(message_state(game)) {
    case MessageStateNoLives:
        if(input.pressed & GameKeyLeft) {
            game_manager_next_level_set(manager, game->levels.menu);
        }
        if(input.pressed & GameKeyRight) {
            game_level_reset(game);
            game_manager_next_level_set(manager, game->levels.game);
        }
        break;
    case MessageStateLevelComplete:
        if(input.pressed & GameKeyLeft) {
            game_level_next(game);
            game_manager_next_level_set(manager, game->levels.menu);
        }
        if(input.pressed & GameKeyRight) {
            game_level_next(game);
            game_level_reset(game);
            game_manager_next_level_set(manager, game->levels.game);
        }
        break;
    case MessageStateGameComplete:
        if(input.pressed & GameKeyLeft) {
            game_level_new_game_plus(game);
            game_manager_next_level_set(manager, game->levels.menu);
        }
        if(input.pressed & GameKeyRight) {
            game_level_new_game_plus(game);
            game_level_reset(game);
            game_manager_next_level_set(manager, game->levels.game);
        }
        break;
    }
}

static void message_render(Entity* self, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(self);
    UNUSED(manager);
    UNUSED(context);

    GameContext* game = game_manager_game_context_get(manager);
    switch(message_state(game)) {
    case MessageStateNoLives:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, "No lives left");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignCenter, "Want to try again?");

        elements_button_left(canvas, "Exit");
        elements_button_right(canvas, "Restart");
        break;
    case MessageStateLevelComplete:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, "Level complete!");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignCenter, "Enter next level?");

        elements_button_left(canvas, "Exit");
        elements_button_right(canvas, "Continue");
        break;
    case MessageStateGameComplete:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, "Game complete!");
        canvas_set_font(canvas, FontSecondary);
        canvas_printf_aligned(
            canvas,
            64,
            30,
            AlignCenter,
            AlignCenter,
            "Continue to NG+ %ld?",
            game->state.new_game_index + 1);

        elements_button_left(canvas, "Exit");
        elements_button_right(canvas, "Continue");
        break;
    }
}

static const EntityDescription message_desc = {
    .start = NULL,
    .stop = NULL,
    .update = message_update,
    .render = message_render,
    .collision = NULL,
    .event = NULL,
    .context_size = 0,
};

static void level_alloc(Level* level, GameManager* manager, void* ctx) {
    UNUSED(manager);
    UNUSED(ctx);
    level_add_entity(level, &message_desc);
}

const LevelBehaviour level_message = {
    .alloc = level_alloc,
    .free = NULL,
    .start = NULL,
    .stop = NULL,
    .context_size = 0,
};