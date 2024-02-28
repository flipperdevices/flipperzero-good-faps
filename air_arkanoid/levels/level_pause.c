#include "level_pause.h"

typedef struct {
    Vector position;
    float speed;
    size_t size;
} Star;

#define STAR_COUNT 50

typedef enum {
    Continue,
    Restart,
    Exit,
} MenuOption;

typedef struct {
    Star stars[STAR_COUNT];
    int selected;
} Menu;

void star_init(Star* star) {
    star->position.x = rand() % 128;
    star->position.y = 0;
    star->speed = ((rand() % 100) / 100.0f) * 2.5f + 0.6f;
    star->size = (rand() % 2);
}

void star_update(Star* star) {
    if(star->position.y > 64) {
        star_init(star);
    }

    star->position.y += star->speed;
}

static void menu_start(Entity* self, GameManager* manager, void* context) {
    UNUSED(self);
    UNUSED(manager);
    Menu* menu = context;
    for(size_t i = 0; i < STAR_COUNT; i++) {
        star_init(&menu->stars[i]);
        for(size_t j = 0; j < 64; j++) {
            star_update(&menu->stars[i]);
        }
    }
    menu->selected = Continue;
}

static void menu_update(Entity* self, GameManager* manager, void* context) {
    UNUSED(self);
    UNUSED(manager);
    Menu* menu = context;
    GameContext* game_context = game_manager_game_context_get(manager);

    for(size_t i = 0; i < STAR_COUNT; i++) {
        star_update(&menu->stars[i]);
    }

    InputState input = game_manager_input_get(manager);
    if(input.pressed & GameKeyUp) {
        menu->selected--;
        if(menu->selected < Continue) {
            menu->selected = Exit;
        }
    }

    if(input.pressed & GameKeyDown) {
        menu->selected++;
        if(menu->selected > Exit) {
            menu->selected = Continue;
        }
    }

    if(input.pressed & GameKeyUp || input.pressed & GameKeyDown || input.pressed & GameKeyOk) {
        game_sound_play(game_context, &sequence_sound_menu);
    }

    if(input.pressed & GameKeyOk) {
        switch(menu->selected) {
        case Continue:
            game_manager_next_level_set(manager, game_context->levels.game);
            break;
        case Restart:
            game_level_reset(game_context);
            game_manager_next_level_set(manager, game_context->levels.game);
            break;
        case Exit:
            game_manager_next_level_set(manager, game_context->levels.menu);
            break;
        default:
            break;
        }

        menu->selected = Continue;
    }
}

static void menu_render(Entity* self, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(self);
    UNUSED(manager);
    Menu* menu = context;
    GameContext* game_context = game_manager_game_context_get(manager);

    // draw stars
    for(size_t i = 0; i < STAR_COUNT; i++) {
        canvas_draw_disc(
            canvas, menu->stars[i].position.x, menu->stars[i].position.y, menu->stars[i].size);
    }

    // draw lives
    for(int32_t i = 0; i < game_context->state.lives; i++) {
        Vector pos = {3 + i * 6, 3};
        canvas_draw_disc(canvas, pos.x, pos.y, 2);
    }

    // draw level and score
    uint32_t level = game_context->state.level_index + 1;
    if(game_context->state.new_game_index > 0) {
        canvas_printf_aligned(
            canvas,
            64,
            0,
            AlignCenter,
            AlignTop,
            "Lvl %ld +%ld",
            level,
            game_context->state.new_game_index);
    } else {
        canvas_printf_aligned(canvas, 64, 0, AlignCenter, AlignTop, "Lvl %ld", level);
    }

    uint32_t score = game_context->state.score;
    canvas_printf_aligned(canvas, 128, 0, AlignRight, AlignTop, "%ld", score);

    // draw menu
    FuriString* str = furi_string_alloc();
    const size_t y_start = 22;
    const size_t y_step = 15;

    furi_string_set(str, menu->selected == Continue ? ">Continue<" : "Continue");
    canvas_draw_str_aligned_outline(
        canvas, 64, y_start + y_step * 0, AlignCenter, AlignCenter, furi_string_get_cstr(str));

    furi_string_set(str, menu->selected == Restart ? ">Restart<" : "Restart");
    canvas_draw_str_aligned_outline(
        canvas, 64, y_start + y_step * 1, AlignCenter, AlignCenter, furi_string_get_cstr(str));

    furi_string_set(str, menu->selected == Exit ? ">Exit<" : "Exit");
    canvas_draw_str_aligned_outline(
        canvas, 64, y_start + y_step * 2, AlignCenter, AlignCenter, furi_string_get_cstr(str));

    furi_string_free(str);
}

static const EntityDescription menu_desc = {
    .start = menu_start,
    .stop = NULL,
    .update = menu_update,
    .render = menu_render,
    .collision = NULL,
    .event = NULL,
    .context_size = sizeof(Menu),
};

static void level_alloc(Level* level, GameManager* manager, void* ctx) {
    UNUSED(manager);
    UNUSED(ctx);
    level_add_entity(level, &menu_desc);
}

static void level_free(Level* level, GameManager* manager, void* ctx) {
    UNUSED(level);
    UNUSED(manager);
    UNUSED(ctx);
}

const LevelBehaviour level_pause = {
    .alloc = level_alloc,
    .free = level_free,
    .start = NULL,
    .stop = NULL,
    .context_size = 0,
};