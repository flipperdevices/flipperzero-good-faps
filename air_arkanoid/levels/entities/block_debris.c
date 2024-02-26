#include "block_debris.h"

typedef struct {
    Vector size;
    Vector speed;
} BlockDebris;

static const EntityDescription block_debris_desc;

void block_debrises_spawn_rect(Level* level, Vector pos, Vector area) {
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

void block_debrises_spawn_round(Level* level, Vector pos, float radius) {
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

void block_debrises_spawn_impact(Level* level, Vector pos, Vector speed) {
    size_t count = rand() % 5 + 2;
    for(size_t i = 0; i < count; i++) {
        Entity* debris = level_add_entity(level, &block_debris_desc);
        entity_pos_set(debris, pos);
        BlockDebris* debris_context = entity_context_get(debris);
        debris_context->size = (Vector){1, 1};

        Vector speed_normal = vector_mul(vector_normalize(speed), -0.75f);
        Vector speed_rand = vector_mul(vector_sub(vector_rand(), 0.5f), 0.4f);
        debris_context->speed = vector_add(speed_normal, speed_rand);
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