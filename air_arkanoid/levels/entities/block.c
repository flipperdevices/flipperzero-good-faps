#include "ball.h"
#include "block.h"
#include "block_debris.h"
#include "../level_message.h"

static size_t level_block_count(Level* level) {
    return level_entity_count(level, &block_desc) + level_entity_count(level, &round_desc);
}

/***** Round block *****/

typedef struct {
    float radius;
    size_t current_lives;
    size_t max_lives;
} Round;

void round_block_spawn(Level* level, Vector pos, float radius, size_t lives) {
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
        Level* level = game_manager_current_level_get(manager);
        GameContext* game = game_manager_game_context_get(manager);

        block_debrises_spawn_impact(level, ball_pos, ball_get_speed(ball));

        // change the ball speed based on the collision
        Vector normal = vector_normalize(vector_sub(ball_pos, round_pos));
        Vector ball_speed = ball_get_speed(ball);
        ball_speed =
            vector_sub(ball_speed, vector_mul(normal, 2 * vector_dot(ball_speed, normal)));

        // ball x speed should be at least 0.1
        if(fabsf(ball_speed.x) < 0.1f) {
            ball_speed.x = ball_speed.x < 0 ? -0.1f : 0.1f;
        }
        ball_set_speed(ball, ball_speed);

        round->current_lives--;

        if(round->current_lives == 0) {
            level_remove_entity(level, self);
        }

        ball_collide(ball, game);

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

const EntityDescription round_desc = {
    .start = NULL,
    .stop = round_stop,
    .update = NULL,
    .render = round_render,
    .collision = round_collision,
    .event = NULL,
    .context_size = sizeof(Round),
};

/***** Rectangular block *****/

typedef struct {
    Vector size;
    size_t current_lives;
    size_t max_lives;
} Block;

void rect_block_spawn(Level* level, Vector pos, Vector size, size_t lives) {
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
        Level* level = game_manager_current_level_get(manager);
        GameContext* game = game_manager_game_context_get(manager);

        block_debrises_spawn_impact(level, ball_pos, ball_get_speed(ball));

        Vector closest = {
            CLAMP(ball_pos.x, block_pos.x + block->size.x / 2, block_pos.x - block->size.x / 2),
            CLAMP(ball_pos.y, block_pos.y + block->size.y / 2, block_pos.y - block->size.y / 2),
        };

        // change the ball speed based on the collision
        Vector distance = vector_sub(ball_pos, closest);
        Vector ball_speed = ball_get_speed(ball);
        if(fabsf(distance.x) < fabsf(distance.y)) {
            ball_speed.y = -ball_speed.y;
        } else {
            ball_speed.x = -ball_speed.x;
        }
        ball_set_speed(ball, ball_speed);

        block->current_lives--;

        if(block->current_lives == 0) {
            level_remove_entity(level, self);
        }

        ball_collide(ball, game);

        if(level_block_count(level) == 0) {
            LevelMessageContext* message_context = level_context_get(game->levels.message);
            furi_string_set(message_context->message, "You win!");
            game_manager_next_level_set(manager, game->levels.message);
            game_sound_play(game, &sequence_level_win);
        }
    }
}

static void block_stop(Entity* self, GameManager* manager, void* context) {
    UNUSED(manager);
    Block* block = context;
    Level* level = game_manager_current_level_get(manager);
    block_debrises_spawn_rect(level, entity_pos_get(self), block->size);
}

const EntityDescription block_desc = {
    .start = NULL,
    .stop = block_stop,
    .update = NULL,
    .render = block_render,
    .collision = block_collision,
    .event = NULL,
    .context_size = sizeof(Block),
};
