#ifndef VOLUME_CONTROL_H
#define VOLUME_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

// Volume control functions
int volume_control_init(void);
int volume_control_cleanup(void);
int volume_control_set_volume(float volume);
float volume_control_get_volume(void);
int volume_control_set_mute(bool mute);
bool volume_control_is_muted(void);
int volume_control_step_up(void);
int volume_control_step_down(void);

// Volume change callback
typedef void (*volume_change_callback_t)(float volume, bool muted);
int volume_control_set_callback(volume_change_callback_t callback);

#endif // VOLUME_CONTROL_H
