#ifndef PLAYBACK_CONTROL_H
#define PLAYBACK_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

// Playback states
typedef enum {
    PLAYBACK_STOPPED,
    PLAYBACK_PLAYING,
    PLAYBACK_PAUSED,
    PLAYBACK_BUFFERING
} playback_state_t;

// Playback control functions
int playback_control_init(void);
int playback_control_cleanup(void);
int playback_control_play(void);
int playback_control_pause(void);
int playback_control_stop(void);
int playback_control_next(void);
int playback_control_previous(void);
playback_state_t playback_control_get_state(void);
int playback_control_set_state(playback_state_t state);

// Playback info
typedef struct {
    char title[256];
    char artist[256];
    char album[256];
    uint32_t duration_ms;
    uint32_t position_ms;
} playback_info_t;

int playback_control_set_info(const playback_info_t *info);
int playback_control_get_info(playback_info_t *info);

// Playback callbacks
typedef void (*playback_state_callback_t)(playback_state_t state);
typedef void (*playback_info_callback_t)(const playback_info_t *info);

int playback_control_set_state_callback(playback_state_callback_t callback);
int playback_control_set_info_callback(playback_info_callback_t callback);

#endif // PLAYBACK_CONTROL_H
