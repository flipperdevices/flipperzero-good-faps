#include "ball.h"
#include "paddle.h"
#include "events.h"

#define WAIT_FRAMES 60

typedef struct {
    Vector size;
    bool ball_launched;
    Entity* ball;
    size_t wait_frames;
} Paddle;

static void paddle_pause(Paddle* paddle) {
    paddle->wait_frames = WAIT_FRAMES;
    Ball* ball = entity_context_get(paddle->ball);
    ball_set_paused(ball, true);
}

void paddle_spawn(Level* level) {
    Entity* paddle = level_add_entity(level, &paddle_desc);
    Paddle* paddle_context = entity_context_get(paddle);
    paddle_context->ball = level_add_entity(level, &ball_desc);
    paddle_pause(paddle_context);
}

static void paddle_start(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Paddle* paddle = context;
    static const Vector paddle_start_size = {30, 3};

    paddle->size = paddle_start_size;
    paddle->ball_launched = false;
    entity_pos_set(self, (Vector){64, 61});
    entity_collider_add_rect(self, paddle->size.x, paddle->size.y);
}

static float paddle_x_from_angle(float angle) {
    const float min_angle = -45.0f;
    const float max_angle = 45.0f;

    return 128.0f * (angle - min_angle) / (max_angle - min_angle);
}

static void paddle_update(Entity* entity, GameManager* manager, void* context) {
    Paddle* paddle = context;
    InputState input = game_manager_input_get(manager);
    GameContext* game_context = game_manager_game_context_get(manager);
    Vector pos = entity_pos_get(entity);
    Ball* ball = entity_context_get(paddle->ball);

    if(!paddle->ball_launched) {
        Vector ball_pos = entity_pos_get(paddle->ball);
        ball_pos.x = pos.x;
        ball_pos.y = pos.y - paddle->size.y / 2 - ball_get_radius(ball);
        entity_pos_set(paddle->ball, ball_pos);
    }

    if(paddle->wait_frames > 0) {
        paddle->wait_frames--;
        return;
    } else {
        ball_set_paused(ball, false);
    }

    float paddle_half = paddle->size.x / 2;
    if(game_context->imu_present) {
        pos.x = paddle_x_from_angle(-imu_pitch_get(game_context->imu));
    } else {
        if(input.held & GameKeyLeft) {
            pos.x -= 2;
        }
        if(input.held & GameKeyRight) {
            pos.x += 2;
        }
    }
    pos.x = CLAMP(pos.x, 128 - paddle_half, paddle_half);
    entity_pos_set(entity, pos);

    if(input.pressed & GameKeyBack) {
        game_manager_next_level_set(manager, game_context->levels.pause);
        paddle_pause(paddle);
    }

    if(input.pressed & GameKeyOk) {
        if(!paddle->ball_launched) {
            paddle->ball_launched = true;
            ball_set_angle(ball, 270.0f);
        }
    }
}

static void paddle_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Paddle* paddle = context;
    Vector pos = entity_pos_get(entity);
    float paddle_half = paddle->size.x / 2;
    canvas_draw_box(canvas, pos.x - paddle_half, pos.y, paddle->size.x, paddle->size.y);

    if(paddle->wait_frames > 0 && paddle->wait_frames != WAIT_FRAMES) {
        FuriString* str = furi_string_alloc_printf("%d", (paddle->wait_frames / 20) + 1);
        const char* cstr = furi_string_get_cstr(str);
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str_aligned_outline(canvas, 64, 32, AlignCenter, AlignCenter, cstr);
        canvas_set_font(canvas, FontSecondary);
        furi_string_free(str);
    }
}

static void paddle_collision(Entity* self, Entity* other, GameManager* manager, void* context) {
    UNUSED(manager);

    if(entity_description_get(other) == &ball_desc) {
        Ball* ball = entity_context_get(other);
        Paddle* paddle = context;
        Vector ball_pos = entity_pos_get(other);
        Vector paddle_pos = entity_pos_get(self);

        float paddle_half = paddle->size.x / 2;
        float paddle_center = paddle_pos.x;
        float paddle_edge = paddle_center - paddle_half;
        float paddle_edge_distance = ball_pos.x - paddle_edge;
        float paddle_edge_distance_normalized = paddle_edge_distance / paddle->size.x;

        // lerp the angle based on the distance from the paddle center
        float angle = 270.0f - 45.0f + 90.0f * paddle_edge_distance_normalized;
        ball_set_angle(ball, angle);
        GameContext* game = game_manager_game_context_get(manager);
        ball_collide_paddle(ball, game);
    }
}

static void paddle_event(Entity* self, GameManager* manager, EntityEvent event, void* context) {
    UNUSED(self);
    if(event.type == GameEventBallLost) {
        Paddle* paddle = context;
        paddle->ball_launched = false;
        Ball* ball = entity_context_get(paddle->ball);
        GameContext* game = game_manager_game_context_get(manager);
        ball_reset(ball, game);

        game->state.lives--;
        if(game->state.lives < 0) {
            game_sound_play(game, &sequence_sound_level_fail);
            game_manager_next_level_set(manager, game->levels.message);
        } else {
            game_sound_play(game, &sequence_sound_ball_lost);
        }
    }
}

const EntityDescription paddle_desc = {
    .start = paddle_start,
    .stop = NULL,
    .update = paddle_update,
    .render = paddle_render,
    .collision = paddle_collision,
    .event = paddle_event,
    .context_size = sizeof(Paddle),
};