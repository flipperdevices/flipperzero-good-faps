#include "level_game.h"
#include "entities/block.h"
#include "entities/paddle.h"
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
            data += sizeof(SerializedBlock);

            Vector block_size = {block->width, block->height};
            Vector block_pos = {
                (float)block->x + block_size.x / 2, (float)block->y + block_size.y / 2};
            rect_block_spawn(level, block_pos, block_size, block->lives);
        } else if(type == SerializedTypePaddle) {
            SerializedRound* round = (SerializedRound*)data;
            data += sizeof(SerializedRound);

            float radius = (float)round->diameter / 2.0f;
            Vector pos = {round->x, round->y};
            pos = vector_add(pos, roundf(radius));
            round_block_spawn(level, pos, radius, round->lives);
        } else {
            FURI_LOG_E(tag, "Invalid entity type");
            return;
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