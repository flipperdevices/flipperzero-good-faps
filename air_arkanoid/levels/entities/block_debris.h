#pragma once
#include "../../game.h"

#ifdef __cplusplus
extern "C" {
#endif

void block_debrises_spawn_round(Level* level, Vector pos, float radius);

void block_debrises_spawn_rect(Level* level, Vector pos, Vector area);

void block_debrises_spawn_impact(Level* level, Vector pos, Vector speed);

#ifdef __cplusplus
}
#endif