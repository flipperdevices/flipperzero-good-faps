#include "sounds.h"

static const NotificationSequence sequence_sound_ball_collide_0 = {
    &message_note_c6,
    &message_delay_10,
    &message_note_c7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_1 = {
    &message_note_d6,
    &message_delay_10,
    &message_note_d7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_2 = {
    &message_note_e6,
    &message_delay_10,
    &message_note_e7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_3 = {
    &message_note_f6,
    &message_delay_10,
    &message_note_f7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_4 = {
    &message_note_g6,
    &message_delay_10,
    &message_note_g7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_5 = {
    &message_note_a6,
    &message_delay_10,
    &message_note_a7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_6 = {
    &message_note_b6,
    &message_delay_10,
    &message_note_b7,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_7 = {
    &message_note_c7,
    &message_delay_10,
    &message_note_c8,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_8 = {
    &message_note_d7,
    &message_delay_10,
    &message_note_d8,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_sound_ball_collide_9 = {
    &message_note_e7,
    &message_delay_10,
    &message_note_e8,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

static const NotificationSequence* sequence_sound_ball_collide[] = {
    &sequence_sound_ball_collide_0,
    &sequence_sound_ball_collide_1,
    &sequence_sound_ball_collide_2,
    &sequence_sound_ball_collide_3,
    &sequence_sound_ball_collide_4,
    &sequence_sound_ball_collide_5,
    &sequence_sound_ball_collide_6,
    &sequence_sound_ball_collide_7,
    &sequence_sound_ball_collide_8,
    &sequence_sound_ball_collide_9,
};

const NotificationSequence* ball_get_collide_sound(size_t collision_count) {
    const size_t max = sizeof(sequence_sound_ball_collide) / sizeof(NotificationSequence*);
    if(collision_count > max) {
        collision_count = max;
    }
    return sequence_sound_ball_collide[collision_count];
}

const NotificationSequence sequence_sound_ball_paddle_collide = {
    &message_note_d6,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

const NotificationSequence sequence_sound_ball_lost = {
    &message_vibro_on,

    &message_note_ds4,
    &message_delay_10,
    &message_sound_off,
    &message_delay_10,

    &message_note_ds4,
    &message_delay_10,
    &message_sound_off,
    &message_delay_10,

    &message_note_ds4,
    &message_delay_10,
    &message_sound_off,
    &message_delay_10,

    &message_vibro_off,
    NULL,
};

const NotificationSequence sequence_level_win = {
    &message_note_c5,
    &message_delay_100,
    &message_note_e5,
    &message_delay_100,
    &message_note_f5,
    &message_delay_100,
    &message_note_g5,
    &message_delay_250,
    &message_sound_off,
    NULL,
};