#include "ball.h"
#include "events.h"
#include "paddle.h"
#include "block_debris.h"
#include "../../game.h"
#include "../level_game.h"

struct Ball {
    Vector speed;
    float radius;
    float max_speed;
    size_t collision_count;
    bool paused;
};

void ball_reset(Ball* ball, GameContext* game) {
    ball->max_speed = 2 + (float)game->state.new_game_index * 0.5f;
    ball->speed = (Vector){0, 0};
    ball->radius = 2;
    ball->collision_count = 0 + game->state.new_game_index;
    ball->paused = false;
}

void ball_set_paused(Ball* ball, bool paused) {
    ball->paused = paused;
}

void ball_collide(Entity* entity, GameContext* game_context) {
    Ball* ball = entity_context_get(entity);
    game_sound_play(game_context, ball_get_collide_sound(ball->collision_count));
    ball->collision_count++;
    game_context->state.score += MIN(ball->collision_count, 10U);
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
    ball_reset(ball, game_manager_game_context_get(manager));
    entity_collider_add_circle(self, ball->radius);
}

void ball_set_angle(Ball* ball, float angle) {
    ball->speed.x = cosf(angle * (M_PI / 180.0f)) * ball->max_speed;
    ball->speed.y = sinf(angle * (M_PI / 180.0f)) * ball->max_speed;
}

static void ball_update(Entity* entity, GameManager* manager, void* context) {
    UNUSED(manager);
    Ball* ball = context;

    if(ball->paused) {
        return;
    }

    Vector pos = entity_pos_get(entity);
    pos = vector_add(pos, ball->speed);

    const Vector screen = {128, 64};

    GameContext* game = game_manager_game_context_get(manager);

    // prevent to go out of screen
    if(pos.x - ball->radius < 0) {
        block_debrises_spawn_impact(game->levels.game, pos, ball_get_speed(ball));
        game_sound_play(game, ball_get_collide_sound(ball->collision_count));
        pos.x = ball->radius;
        ball->speed.x = -ball->speed.x;
    } else if(pos.x + ball->radius > screen.x) {
        block_debrises_spawn_impact(game->levels.game, pos, ball_get_speed(ball));
        game_sound_play(game, ball_get_collide_sound(ball->collision_count));
        pos.x = screen.x - ball->radius;
        ball->speed.x = -ball->speed.x;
    } else if(pos.y - ball->radius < 0) {
        block_debrises_spawn_impact(game->levels.game, pos, ball_get_speed(ball));
        game_sound_play(game, ball_get_collide_sound(ball->collision_count));
        pos.y = ball->radius;
        ball->speed.y = -ball->speed.y;
    } else if(pos.y - ball->radius > screen.y) {
        level_send_event(
            game->levels.game, entity, &paddle_desc, GameEventBallLost, (EntityEventValue){0});
    }

    entity_pos_set(entity, pos);
}

static void ball_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Ball* ball = context;
    Vector pos = entity_pos_get(entity);
    canvas_draw_disc(canvas, pos.x, pos.y, ball->radius);

    if(ball->paused) {
        float factor = 5 + ball->radius;
        Vector speed = vector_mul(vector_normalize(ball->speed), factor);
        canvas_draw_line(canvas, pos.x, pos.y, pos.x + speed.x, pos.y + speed.y);
    }
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