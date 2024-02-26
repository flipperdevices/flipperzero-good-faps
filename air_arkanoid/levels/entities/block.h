#pragma once
#include "../../game.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const EntityDescription round_desc;
extern const EntityDescription block_desc;

void round_block_spawn(Level* level, Vector pos, float radius, size_t lives);

void rect_block_spawn(Level* level, Vector pos, Vector size, size_t lives);

#ifdef __cplusplus
}
#endif