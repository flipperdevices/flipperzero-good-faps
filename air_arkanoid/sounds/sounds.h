#pragma once
#include <notification/notification_messages.h>

const NotificationSequence* ball_get_collide_sound(size_t collision_count);

extern const NotificationSequence sequence_sound_ball_paddle_collide;

extern const NotificationSequence sequence_sound_ball_lost;

extern const NotificationSequence sequence_level_win;