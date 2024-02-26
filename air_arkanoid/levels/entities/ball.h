#pragma once
#include "../../game.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const EntityDescription ball_desc;

typedef struct Ball Ball;

void ball_set_angle(Ball* ball, float angle);

void ball_reset(Ball* ball);

void ball_collide(Ball* ball, GameContext* game_context);

void ball_collide_paddle(Ball* ball, GameContext* game_context);

Vector ball_get_speed(Ball* ball);

void ball_set_speed(Ball* ball, Vector speed);

float ball_get_radius(Ball* ball);

#ifdef __cplusplus
}
#endif