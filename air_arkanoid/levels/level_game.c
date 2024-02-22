#include "level_game.h"
#include "level_message.h"

typedef enum {
    GameEventBallLost,
} GameEvent;

/****** Ball ******/

static const EntityDescription paddle_desc;

typedef struct {
    Vector speed;
    float radius;
    float max_speed;
    size_t collision_count;
} Ball;

static void ball_reset(Ball* ball) {
    ball->max_speed = 2;
    ball->speed = (Vector){0, 0};
    ball->radius = 2;
    ball->collision_count = 0;
}

static void ball_start(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Ball* ball = context;
    ball_reset(ball);
    entity_collider_add_circle(self, ball->radius);
}

static void ball_set_angle(Ball* ball, float angle) {
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

static const EntityDescription ball_desc = {
    .start = ball_start,
    .stop = NULL,
    .update = ball_update,
    .render = ball_render,
    .collision = NULL,
    .event = NULL,
    .context_size = sizeof(Ball),
};

/****** Block Debris ******/
static const EntityDescription block_debris_desc;

typedef struct {
    Vector size;
    Vector speed;
} BlockDebris;

Vector vector_rand() {
    float x = (rand() % __INT_MAX__) / (float)__INT_MAX__;
    float y = (rand() % __INT_MAX__) / (float)__INT_MAX__;
    return (Vector){x - 0.5f, y - 0.5f};
}

static void block_debrises_spawn(Level* level, Vector pos, Vector area) {
    size_t count = rand() % 5 + 5;
    for(size_t i = 0; i < count; i++) {
        Entity* debris = level_add_entity(level, &block_debris_desc);
        Vector new_pos = {
            pos.x - (area.x / 2) + (rand() % (int)area.x),
            pos.y - (area.y / 2) + (rand() % (int)area.y),
        };

        entity_pos_set(debris, new_pos);
        BlockDebris* debris_context = entity_context_get(debris);
        debris_context->size = (Vector){rand() % 2 + 1, rand() % 2 + 1};
        debris_context->speed = vector_rand();
    }
}

static void block_debris_update(Entity* entity, GameManager* manager, void* context) {
    BlockDebris* block = context;
    Vector pos = entity_pos_get(entity);
    pos = vector_add(pos, block->speed);
    entity_pos_set(entity, pos);
    block->speed.y *= 0.9f;
    block->speed.x *= 0.9f;

    if(fabsf(block->speed.x) < 0.1f && fabsf(block->speed.y) < 0.1f) {
        Level* level = game_manager_current_level_get(manager);
        level_remove_entity(level, entity);
    }
}

static void
    block_debris_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    BlockDebris* block = context;
    Vector pos = entity_pos_get(entity);
    canvas_draw_box(
        canvas, pos.x - block->size.x / 2, pos.y - block->size.y / 2, block->size.x, block->size.y);
}

static const EntityDescription block_debris_desc = {
    .start = NULL,
    .stop = NULL,
    .update = block_debris_update,
    .render = block_debris_render,
    .collision = NULL,
    .event = NULL,
    .context_size = sizeof(BlockDebris),
};

/****** Block ******/

static const EntityDescription block_desc;

typedef struct {
    Vector size;
} Block;

static void block_spawn(Level* level, Vector pos, Vector size) {
    Entity* block = level_add_entity(level, &block_desc);
    entity_collider_add_rect(block, size.x, size.y);
    entity_pos_set(block, pos);
    Block* block_context = entity_context_get(block);
    block_context->size = size;
}

static void block_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Block* block = context;
    Vector pos = entity_pos_get(entity);
    canvas_draw_box(
        canvas, pos.x - block->size.x / 2, pos.y - block->size.y / 2, block->size.x, block->size.y);
}

static void block_collision(Entity* self, Entity* other, GameManager* manager, void* context) {
    UNUSED(manager);

    if(entity_description_get(other) == &ball_desc) {
        Ball* ball = entity_context_get(other);
        Block* block = context;
        Vector ball_pos = entity_pos_get(other);
        Vector block_pos = entity_pos_get(self);

        Vector closest = {
            CLAMP(ball_pos.x, block_pos.x + block->size.x / 2, block_pos.x - block->size.x / 2),
            CLAMP(ball_pos.y, block_pos.y + block->size.y / 2, block_pos.y - block->size.y / 2),
        };

        // change the ball speed based on the collision
        Vector distance = vector_sub(ball_pos, closest);
        if(fabsf(distance.x) < fabsf(distance.y)) {
            ball->speed.y = -ball->speed.y;
        } else {
            ball->speed.x = -ball->speed.x;
        }

        Level* level = game_manager_current_level_get(manager);
        level_remove_entity(level, self);

        GameContext* game = game_manager_game_context_get(manager);
        game_sound_play(game, ball_get_collide_sound(ball->collision_count));
        ball->collision_count++;

        block_debrises_spawn(level, block_pos, block->size);

        if(level_entity_count(level, &block_desc) == 0) {
            LevelMessageContext* message_context = level_context_get(game->levels.message);
            furi_string_set(message_context->message, "You win!");
            game_manager_next_level_set(manager, game->levels.message);
            game_sound_play(game, &sequence_level_win);
        }
    }
}

static const EntityDescription block_desc = {
    .start = NULL,
    .stop = NULL,
    .update = NULL,
    .render = block_render,
    .collision = block_collision,
    .event = NULL,
    .context_size = sizeof(Block),
};

/****** Paddle ******/

static const Vector paddle_start_size = {30, 3};

typedef struct {
    Vector size;
    bool ball_launched;
    Entity* ball;
} Paddle;

static void paddle_start(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Paddle* paddle = context;
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
        ball_pos.y = pos.y - paddle->size.y / 2 - ball->radius;
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
        ball->collision_count = 0;

        GameContext* game = game_manager_game_context_get(manager);
        game_sound_play(game, &sequence_sound_ball_paddle_collide);
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

static const EntityDescription paddle_desc = {
    .start = paddle_start,
    .stop = paddle_stop,
    .update = paddle_update,
    .render = paddle_render,
    .collision = paddle_collision,
    .event = paddle_event,
    .context_size = sizeof(Paddle),
};

static void level_1_spawn(Level* level) {
    level_add_entity(level, &paddle_desc);
    const Vector block_size = {13, 5};
    const Vector screen = {128, 64};
    const int block_count_x = screen.x / block_size.x;
    const int block_count_y = 6;
    size_t block_spacing = 1;

    for(int y = 0; y < block_count_y; y++) {
        for(int x = 0; x < block_count_x; x++) {
            Vector pos = {
                (x) * (block_size.x + block_spacing) + block_size.x / 2,
                (y) * (block_size.y + block_spacing) + block_size.y / 2,
            };
            block_spawn(level, pos, block_size);
        }
    }
}

static void level_game_start(Level* level, GameManager* manager, void* context) {
    UNUSED(manager);
    UNUSED(context);
    level_1_spawn(level);
}

static void level_game_stop(Level* level, GameManager* manager, void* context) {
    UNUSED(manager);
    UNUSED(context);
    level_clear(level);
}

const LevelBehaviour level_game = {
    .alloc = NULL,
    .free = NULL,
    .start = level_game_start,
    .stop = level_game_stop,
    .context_size = 0,
};