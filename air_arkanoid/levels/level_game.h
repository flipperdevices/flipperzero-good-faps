#pragma once
#include "../game.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const LevelBehaviour level_game;

void level_load(Level* level, const char* path);

#ifdef __cplusplus
}
#endif