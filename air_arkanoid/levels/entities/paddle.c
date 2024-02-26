#include "ball.h"
#include "paddle.h"
#include "events.h"

typedef struct {
    Vector size;
    bool ball_launched;
    Entity* ball;
} Paddle;

static void paddle_start(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Paddle* paddle = context;
    static const Vector paddle_start_size = {30, 3};

    paddle->size = paddle_start_size;
    paddle->ball_launched = false;
    entity_pos_set(self, (Vector){64, 61});
    entity_collider_add_rect(self, paddle->size.x, paddle->size.y);

    Level* level = game_manager_current_level_get(manager);
    paddle->ball = level_add_entity(level, &ball_desc);
}

static void paddle_stop(Entity* entity, GameManager* manager, void* context) {
    UNUSED(entity);
    Paddle* paddle = context;

    Level* level = game_manager_current_level_get(manager);
    level_remove_entity(level, paddle->ball);
    paddle->ball = NULL;
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
        game_manager_next_level_set(manager, game_context->levels.menu);
    }

    if(input.pressed & GameKeyOk) {
        if(!paddle->ball_launched) {
            paddle->ball_launched = true;

            Ball* ball = entity_context_get(paddle->ball);
            ball_set_angle(ball, 270.0f);
        }
    }

    if(!paddle->ball_launched) {
        Vector ball_pos = entity_pos_get(paddle->ball);
        Ball* ball = entity_context_get(paddle->ball);
        ball_pos.x = pos.x;
        ball_pos.y = pos.y - paddle->size.y / 2 - ball_get_radius(ball);
        entity_pos_set(paddle->ball, ball_pos);
    }
}

static void paddle_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Paddle* paddle = context;
    Vector pos = entity_pos_get(entity);
    float paddle_half = paddle->size.x / 2;
    canvas_draw_box(canvas, pos.x - paddle_half, pos.y, paddle->size.x, paddle->size.y);
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
    UNUSED(manager);
    UNUSED(self);
    if(event.type == GameEventBallLost) {
        Paddle* paddle = context;
        paddle->ball_launched = false;
        Ball* ball = entity_context_get(paddle->ball);
        ball_reset(ball);
        GameContext* game = game_manager_game_context_get(manager);
        game_sound_play(game, &sequence_sound_ball_lost);
    }
}

const EntityDescription paddle_desc = {
    .start = paddle_start,
    .stop = paddle_stop,
    .update = paddle_update,
    .render = paddle_render,
    .collision = paddle_collision,
    .event = paddle_event,
    .context_size = sizeof(Paddle),
};