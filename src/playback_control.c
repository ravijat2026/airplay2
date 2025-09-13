#include "playback_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>

static pthread_mutex_t playback_mutex = PTHREAD_MUTEX_INITIALIZER;
static playback_state_t current_state = PLAYBACK_STOPPED;
static playback_info_t current_info;
static playback_state_callback_t state_callback = NULL;
static playback_info_callback_t info_callback = NULL;

int playback_control_init(void) {
    pthread_mutex_lock(&playback_mutex);
    
    current_state = PLAYBACK_STOPPED;
    memset(&current_info, 0, sizeof(current_info));
    
    pthread_mutex_unlock(&playback_mutex);
    
    syslog(LOG_INFO, "Playback control initialized");
    return 0;
}

int playback_control_cleanup(void) {
    pthread_mutex_lock(&playback_mutex);
    
    state_callback = NULL;
    info_callback = NULL;
    
    pthread_mutex_unlock(&playback_mutex);
    
    syslog(LOG_INFO, "Playback control cleaned up");
    return 0;
}

int playback_control_play(void) {
    pthread_mutex_lock(&playback_mutex);
    
    if (current_state == PLAYBACK_PAUSED) {
        current_state = PLAYBACK_PLAYING;
        syslog(LOG_INFO, "Playback resumed");
    } else if (current_state == PLAYBACK_STOPPED) {
        current_state = PLAYBACK_PLAYING;
        syslog(LOG_INFO, "Playback started");
    }
    
    // Notify callback
    if (state_callback) {
        state_callback(current_state);
    }
    
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

int playback_control_pause(void) {
    pthread_mutex_lock(&playback_mutex);
    
    if (current_state == PLAYBACK_PLAYING) {
        current_state = PLAYBACK_PAUSED;
        syslog(LOG_INFO, "Playback paused");
        
        // Notify callback
        if (state_callback) {
            state_callback(current_state);
        }
    }
    
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

int playback_control_stop(void) {
    pthread_mutex_lock(&playback_mutex);
    
    current_state = PLAYBACK_STOPPED;
    current_info.position_ms = 0;
    
    syslog(LOG_INFO, "Playback stopped");
    
    // Notify callbacks
    if (state_callback) {
        state_callback(current_state);
    }
    if (info_callback) {
        info_callback(&current_info);
    }
    
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

int playback_control_next(void) {
    pthread_mutex_lock(&playback_mutex);
    
    syslog(LOG_INFO, "Next track requested");
    
    // Reset position for new track
    current_info.position_ms = 0;
    
    // Notify callback
    if (info_callback) {
        info_callback(&current_info);
    }
    
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

int playback_control_previous(void) {
    pthread_mutex_lock(&playback_mutex);
    
    syslog(LOG_INFO, "Previous track requested");
    
    // Reset position for previous track
    current_info.position_ms = 0;
    
    // Notify callback
    if (info_callback) {
        info_callback(&current_info);
    }
    
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

playback_state_t playback_control_get_state(void) {
    pthread_mutex_lock(&playback_mutex);
    playback_state_t state = current_state;
    pthread_mutex_unlock(&playback_mutex);
    return state;
}

int playback_control_set_state(playback_state_t state) {
    if (state < PLAYBACK_STOPPED || state > PLAYBACK_BUFFERING) {
        return -1;
    }
    
    pthread_mutex_lock(&playback_mutex);
    
    current_state = state;
    
    // Notify callback
    if (state_callback) {
        state_callback(current_state);
    }
    
    pthread_mutex_unlock(&playback_mutex);
    
    syslog(LOG_DEBUG, "Playback state set to %d", state);
    return 0;
}

int playback_control_set_info(const playback_info_t *info) {
    if (!info) {
        return -1;
    }
    
    pthread_mutex_lock(&playback_mutex);
    
    current_info = *info;
    
    // Notify callback
    if (info_callback) {
        info_callback(&current_info);
    }
    
    pthread_mutex_unlock(&playback_mutex);
    
    syslog(LOG_DEBUG, "Playback info updated: %s - %s", 
           info->artist, info->title);
    return 0;
}

int playback_control_get_info(playback_info_t *info) {
    if (!info) {
        return -1;
    }
    
    pthread_mutex_lock(&playback_mutex);
    *info = current_info;
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

int playback_control_set_state_callback(playback_state_callback_t callback) {
    pthread_mutex_lock(&playback_mutex);
    state_callback = callback;
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}

int playback_control_set_info_callback(playback_info_callback_t callback) {
    pthread_mutex_lock(&playback_mutex);
    info_callback = callback;
    pthread_mutex_unlock(&playback_mutex);
    return 0;
}
