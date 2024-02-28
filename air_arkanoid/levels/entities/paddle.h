#pragma once
#include "../../game.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const EntityDescription paddle_desc;

void paddle_spawn(Level* level);

#ifdef __cplusplus
}
#endif