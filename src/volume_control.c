#include "volume_control.h"
#include "audio_output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>

#define VOLUME_STEP 0.05f
#define MIN_VOLUME 0.0f
#define MAX_VOLUME 1.0f

static pthread_mutex_t volume_mutex = PTHREAD_MUTEX_INITIALIZER;
static float current_volume = 0.5f;
static bool is_muted = false;
static volume_change_callback_t volume_callback = NULL;

int volume_control_init(void) {
    pthread_mutex_lock(&volume_mutex);
    
    current_volume = 0.5f;
    is_muted = false;
    
    pthread_mutex_unlock(&volume_mutex);
    
    syslog(LOG_INFO, "Volume control initialized");
    return 0;
}

int volume_control_cleanup(void) {
    pthread_mutex_lock(&volume_mutex);
    
    volume_callback = NULL;
    
    pthread_mutex_unlock(&volume_mutex);
    
    syslog(LOG_INFO, "Volume control cleaned up");
    return 0;
}

int volume_control_set_volume(float volume) {
    if (volume < MIN_VOLUME || volume > MAX_VOLUME) {
        return -1;
    }
    
    pthread_mutex_lock(&volume_mutex);
    
    current_volume = volume;
    
    // Apply volume to audio output
    audio_output_set_volume(is_muted ? 0.0f : current_volume);
    
    // Notify callback
    if (volume_callback) {
        volume_callback(current_volume, is_muted);
    }
    
    pthread_mutex_unlock(&volume_mutex);
    
    syslog(LOG_DEBUG, "Volume set to %.2f", volume);
    return 0;
}

float volume_control_get_volume(void) {
    pthread_mutex_lock(&volume_mutex);
    float volume = current_volume;
    pthread_mutex_unlock(&volume_mutex);
    return volume;
}

int volume_control_set_mute(bool mute) {
    pthread_mutex_lock(&volume_mutex);
    
    is_muted = mute;
    
    // Apply mute to audio output
    audio_output_set_volume(is_muted ? 0.0f : current_volume);
    
    // Notify callback
    if (volume_callback) {
        volume_callback(current_volume, is_muted);
    }
    
    pthread_mutex_unlock(&volume_mutex);
    
    syslog(LOG_DEBUG, "Mute %s", mute ? "enabled" : "disabled");
    return 0;
}

bool volume_control_is_muted(void) {
    pthread_mutex_lock(&volume_mutex);
    bool muted = is_muted;
    pthread_mutex_unlock(&volume_mutex);
    return muted;
}

int volume_control_step_up(void) {
    pthread_mutex_lock(&volume_mutex);
    
    float new_volume = current_volume + VOLUME_STEP;
    if (new_volume > MAX_VOLUME) {
        new_volume = MAX_VOLUME;
    }
    
    current_volume = new_volume;
    
    // Apply volume to audio output
    audio_output_set_volume(is_muted ? 0.0f : current_volume);
    
    // Notify callback
    if (volume_callback) {
        volume_callback(current_volume, is_muted);
    }
    
    pthread_mutex_unlock(&volume_mutex);
    
    syslog(LOG_DEBUG, "Volume stepped up to %.2f", current_volume);
    return 0;
}

int volume_control_step_down(void) {
    pthread_mutex_lock(&volume_mutex);
    
    float new_volume = current_volume - VOLUME_STEP;
    if (new_volume < MIN_VOLUME) {
        new_volume = MIN_VOLUME;
    }
    
    current_volume = new_volume;
    
    // Apply volume to audio output
    audio_output_set_volume(is_muted ? 0.0f : current_volume);
    
    // Notify callback
    if (volume_callback) {
        volume_callback(current_volume, is_muted);
    }
    
    pthread_mutex_unlock(&volume_mutex);
    
    syslog(LOG_DEBUG, "Volume stepped down to %.2f", current_volume);
    return 0;
}

int volume_control_set_callback(volume_change_callback_t callback) {
    pthread_mutex_lock(&volume_mutex);
    volume_callback = callback;
    pthread_mutex_unlock(&volume_mutex);
    return 0;
}
