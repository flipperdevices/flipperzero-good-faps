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

static void block_debrises_spawn_rect(Level* level, Vector pos, Vector area) {
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
        debris_context->speed = vector_sub(vector_rand(), 0.5f);
    }
}

static void block_debrises_spawn_round(Level* level, Vector pos, float radius) {
    size_t count = rand() % 5 + 5;
    for(size_t i = 0; i < count; i++) {
        Entity* debris = level_add_entity(level, &block_debris_desc);
        float angle = (float)(rand() % 360);
        float distance = (float)(rand() % (int)radius);
        Vector new_pos = {
            pos.x + cosf(angle * (M_PI / 180.0f)) * distance,
            pos.y + sinf(angle * (M_PI / 180.0f)) * distance,
        };

        entity_pos_set(debris, new_pos);
        BlockDebris* debris_context = entity_context_get(debris);
        debris_context->size = (Vector){rand() % 2 + 1, rand() % 2 + 1};
        debris_context->speed = vector_sub(vector_rand(), 0.5f);
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

/****** Round Block ******/

static const EntityDescription round_desc;
static const EntityDescription block_desc;

size_t level_block_count(Level* level) {
    return level_entity_count(level, &block_desc) + level_entity_count(level, &round_desc);
}

typedef struct {
    float radius;
    size_t current_lives;
    size_t max_lives;
} Round;

static void round_spawn(Level* level, Vector pos, float radius, size_t lives) {
    Entity* round = level_add_entity(level, &round_desc);
    entity_collider_add_circle(round, radius);
    entity_pos_set(round, pos);
    Round* round_context = entity_context_get(round);
    round_context->radius = radius;
    round_context->max_lives = lives;
    round_context->current_lives = round_context->max_lives;
}

static void round_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Round* round = context;
    Vector pos = entity_pos_get(entity);
    float r = round->radius - 0.5f;
    canvas_draw_disc(canvas, pos.x - 1, pos.y - 1, r);

    size_t lives_diff = round->max_lives - round->current_lives;
    if(lives_diff > 0) {
        canvas_set_color(canvas, ColorWhite);
        float pixel_count = M_PI * r * r;
        pixel_count = pixel_count * lives_diff / round->max_lives;
        for(size_t i = 0; i < pixel_count; i++) {
            // draw random pixels to simulate damage
            canvas_draw_dot(
                canvas,
                pos.x - round->radius + (rand() % ((int)round->radius * 2) + 1),
                pos.y - round->radius + (rand() % ((int)round->radius * 2)) + 1);
        }
        canvas_set_color(canvas, ColorBlack);
    }
}

static void round_collision(Entity* self, Entity* other, GameManager* manager, void* context) {
    UNUSED(manager);

    if(entity_description_get(other) == &ball_desc) {
        Ball* ball = entity_context_get(other);
        Round* round = context;
        Vector ball_pos = entity_pos_get(other);
        Vector round_pos = entity_pos_get(self);

        // change the ball speed based on the collision
        Vector normal = vector_normalize(vector_sub(ball_pos, round_pos));
        ball->speed =
            vector_sub(ball->speed, vector_mul(normal, 2 * vector_dot(ball->speed, normal)));

        // ball x speed should be at least 0.1
        if(fabsf(ball->speed.x) < 0.1f) {
            ball->speed.x = ball->speed.x < 0 ? -0.1f : 0.1f;
        }

        round->current_lives--;

        Level* level = game_manager_current_level_get(manager);
        GameContext* game = game_manager_game_context_get(manager);

        if(round->current_lives == 0) {
            level_remove_entity(level, self);
        }

        game_sound_play(game, ball_get_collide_sound(ball->collision_count));
        ball->collision_count++;

        if(level_block_count(level) == 0) {
            LevelMessageContext* message_context = level_context_get(game->levels.message);
            furi_string_set(message_context->message, "You win!");
            game_manager_next_level_set(manager, game->levels.message);
            game_sound_play(game, &sequence_level_win);
        }
    }
}

static void round_stop(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Round* round = context;
    Level* level = game_manager_current_level_get(manager);
    block_debrises_spawn_round(level, entity_pos_get(self), round->radius);
}

static const EntityDescription round_desc = {
    .start = NULL,
    .stop = round_stop,
    .update = NULL,
    .render = round_render,
    .collision = round_collision,
    .event = NULL,
    .context_size = sizeof(Round),
};

/****** Block ******/

typedef struct {
    Vector size;
    size_t current_lives;
    size_t max_lives;
} Block;

static void block_spawn(Level* level, Vector pos, Vector size, size_t lives) {
    Entity* block = level_add_entity(level, &block_desc);
    entity_collider_add_rect(block, size.x, size.y);
    entity_pos_set(block, pos);
    Block* block_context = entity_context_get(block);
    block_context->size = size;
    block_context->max_lives = lives;
    block_context->current_lives = block_context->max_lives;
}

static void block_render(Entity* entity, GameManager* manager, Canvas* canvas, void* context) {
    UNUSED(manager);
    Block* block = context;
    Vector pos = entity_pos_get(entity);
    pos = vector_sub(pos, vector_div(block->size, 2));
    canvas_draw_box(canvas, pos.x, pos.y, block->size.x, block->size.y);

    size_t lives_diff = block->max_lives - block->current_lives;
    if(lives_diff > 0) {
        canvas_set_color(canvas, ColorWhite);
        // draw random pixels to simulate damage
        float pixel_count = block->size.x * block->size.y;
        pixel_count = pixel_count * lives_diff / block->max_lives;
        for(size_t i = 0; i < pixel_count; i++) {
            canvas_draw_dot(
                canvas,
                pos.x + (rand() % (int)block->size.x),
                pos.y + (rand() % (int)block->size.y));
        }
        canvas_set_color(canvas, ColorBlack);
    }
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

        block->current_lives--;

        Level* level = game_manager_current_level_get(manager);
        GameContext* game = game_manager_game_context_get(manager);

        if(block->current_lives == 0) {
            level_remove_entity(level, self);
        }

        game_sound_play(game, ball_get_collide_sound(ball->collision_count));
        ball->collision_count++;

        if(level_block_count(level) == 0) {
            LevelMessageContext* message_context = level_context_get(game->levels.message);
            furi_string_set(message_context->message, "You win!");
            game_manager_next_level_set(manager, game->levels.message);
            game_sound_play(game, &sequence_level_win);
        }
    }
}

void block_stop(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Block* block = context;
    Level* level = game_manager_current_level_get(manager);
    block_debrises_spawn_rect(level, entity_pos_get(self), block->size);
}

static const EntityDescription block_desc = {
    .start = NULL,
    .stop = block_stop,
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

#include <storage/storage.h>

typedef enum {
    SerializedTypeBlock = 0xFF,
    SerializedTypePaddle = 0xFE,
} SerializedType;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint8_t lives;
} __attribute__((packed)) SerializedBlock;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t diameter;
    uint8_t lives;
} __attribute__((packed)) SerializedRound;

static void level_deserialize(Level* level, const uint8_t* data) {
    const uint8_t magic[] = {0x46, 0x4C, 0x41, 0x41};
    const uint8_t version = 1;
    const char tag[] = "Level loader";

    if(memcmp(data, magic, sizeof(magic)) != 0) {
        FURI_LOG_E(tag, "Invalid magic");
        return;
    }
    data += 4;

    if(*data != version) {
        FURI_LOG_E(tag, "Invalid version");
        return;
    }
    data++;

    size_t entity_count = *data;
    data++;

    for(size_t i = 0; i < entity_count; i++) {
        const uint8_t type = *data;
        data++;

        if(type == SerializedTypeBlock) {
            SerializedBlock* block = (SerializedBlock*)data;
            Vector block_size = {block->width, block->height};
            Vector block_pos = {
                (float)block->x + block_size.x / 2, (float)block->y + block_size.y / 2};
            FURI_LOG_I(
                tag,
                "Block: x %.2f, y %.2f, width %.2f, height %.2f, lives %d",
                (double)block_pos.x,
                (double)block_pos.y,
                (double)block_size.x,
                (double)block_size.y,
                block->lives);
            block_spawn(level, block_pos, block_size, block->lives);
            data += sizeof(SerializedBlock);
        } else if(type == SerializedTypePaddle) {
            SerializedRound* round = (SerializedRound*)data;
            float radius = (float)round->diameter / 2.0f;
            Vector pos = {round->x, round->y};
            pos = vector_add(pos, roundf(radius));
            FURI_LOG_I(
                tag,
                "Round: x %.2f, y %.2f, radius %.2f, lives %d",
                (double)pos.x,
                (double)pos.y,
                (double)radius,
                round->lives);
            round_spawn(level, pos, radius, round->lives);
            data += sizeof(SerializedRound);
        }
    }
}

static void level_load(Level* level, const char* name) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    uint8_t* data = NULL;
    size_t size = 0;

    do {
        if(!storage_file_open(file, name, FSAM_READ, FSOM_OPEN_EXISTING)) {
            break;
        }

        size = storage_file_size(file);
        data = malloc(size);

        if(storage_file_read(file, data, size) != size) {
            break;
        }

        level_clear(level);
        level_deserialize(level, data);
        level_add_entity(level, &paddle_desc);
    } while(false);

    free(data);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void level_game_start(Level* level, GameManager* manager, void* context) {
    UNUSED(manager);
    UNUSED(context);
    // level_1_spawn(level);
    level_load(level, APP_ASSETS_PATH("levels/1.flaam"));
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