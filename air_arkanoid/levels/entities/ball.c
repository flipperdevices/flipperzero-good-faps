#include "ball.h"
#include "events.h"
#include "paddle.h"
#include "../../game.h"

struct Ball {
    Vector speed;
    float radius;
    float max_speed;
    size_t collision_count;
};

void ball_reset(Ball* ball) {
    ball->max_speed = 2;
    ball->speed = (Vector){0, 0};
    ball->radius = 2;
    ball->collision_count = 0;
}

void ball_collide(Ball* ball, GameContext* game_context) {
    game_sound_play(game_context, ball_get_collide_sound(ball->collision_count));
    ball->collision_count++;
}

void ball_collide_paddle(Ball* ball, GameContext* game_context) {
    ball->collision_count = 0;
    game_sound_play(game_context, &sequence_sound_ball_paddle_collide);
}

Vector ball_get_speed(Ball* ball) {
    return ball->speed;
}

void ball_set_speed(Ball* ball, Vector speed) {
    ball->speed = speed;
}

float ball_get_radius(Ball* ball) {
    return ball->radius;
}

static void ball_start(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Ball* ball = context;
    ball_reset(ball);
    entity_collider_add_circle(self, ball->radius);
}

void ball_set_angle(Ball* ball, float angle) {
    ball->speed.x = cosf(angle * (M_PI / 180.0f)) * ball->max_speed;
    ball->speed.y = sinf(angle * (M_PI / 180.0f)) * ball->max_speed;
}

static void ball_update(Entity* entity, GameManager* manager, void* context) {
    UNUSED(manager);
    Ball* ball = context;
    Vector pos = entity_pos_get(entity);
    pos = vector_add(pos, ball->speed);

    const Vector screen = {128, 64};

    // prevent to go out of screen
    if(pos.x - ball->radius < 0) {
        pos.x = ball->radius;
        ball->speed.x = -ball->speed.x;
    } else if(pos.x + ball->radius > screen.x) {
        pos.x = screen.x - ball->radius;
        ball->speed.x = -ball->speed.x;
    } else if(pos.y - ball->radius < 0) {
        pos.y = ball->radius;
        ball->speed.y = -ball->speed.y;
    } else if(pos.y - ball->radius > screen.y) {
        Level* level = game_manager_current_level_get(manager);
        level_send_event(level, entity, &paddle_desc, GameEventBallLost, (EntityEventValue){0});
    }

    entity_pos_set(entity, pos);
}

static void ball_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Ball* ball = context;
    Vector pos = entity_pos_get(entity);
    canvas_draw_disc(canvas, pos.x, pos.y, ball->radius);
}

const EntityDescription ball_desc = {
    .start = ball_start,
    .stop = NULL,
    .update = ball_update,
    .render = ball_render,
    .collision = NULL,
    .event = NULL,
    .context_size = sizeof(Ball),
};