#include "audio_output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_CHANNELS 2
#define DEFAULT_BITS_PER_SAMPLE 16
#define DEFAULT_BUFFER_SIZE 4096

static snd_pcm_t *pcm_handle = NULL;
static audio_config_t current_config;
static bool is_running = false;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t *audio_buffer = NULL;
static size_t buffer_size = DEFAULT_BUFFER_SIZE;
static size_t buffer_write_pos = 0;
static size_t buffer_read_pos = 0;

int audio_output_init(void) {
    pthread_mutex_lock(&audio_mutex);
    
    // Set default configuration
    current_config.sample_rate = DEFAULT_SAMPLE_RATE;
    current_config.channels = DEFAULT_CHANNELS;
    current_config.bits_per_sample = DEFAULT_BITS_PER_SAMPLE;
    current_config.device_name = "default";
    current_config.use_hw_volume = false;
    
    // Allocate audio buffer
    audio_buffer = malloc(buffer_size);
    if (!audio_buffer) {
        syslog(LOG_ERR, "Failed to allocate audio buffer");
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    memset(audio_buffer, 0, buffer_size);
    buffer_write_pos = 0;
    buffer_read_pos = 0;
    
    pthread_mutex_unlock(&audio_mutex);
    
    syslog(LOG_INFO, "Audio output initialized");
    return 0;
}

int audio_output_cleanup(void) {
    pthread_mutex_lock(&audio_mutex);
    
    if (is_running) {
        audio_output_stop();
    }
    
    if (pcm_handle) {
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
    
    if (audio_buffer) {
        free(audio_buffer);
        audio_buffer = NULL;
    }
    
    pthread_mutex_unlock(&audio_mutex);
    
    syslog(LOG_INFO, "Audio output cleaned up");
    return 0;
}

int audio_output_configure(const audio_config_t *config) {
    if (!config) {
        return -1;
    }
    
    pthread_mutex_lock(&audio_mutex);
    
    current_config = *config;
    
    pthread_mutex_unlock(&audio_mutex);
    
    syslog(LOG_INFO, "Audio output configured: %dHz, %d channels, %d bits",
           config->sample_rate, config->channels, config->bits_per_sample);
    return 0;
}

int audio_output_start(void) {
    pthread_mutex_lock(&audio_mutex);
    
    if (is_running) {
        pthread_mutex_unlock(&audio_mutex);
        return 0;
    }
    
    // Open PCM device
    int err = snd_pcm_open(&pcm_handle, current_config.device_name, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot open PCM device: %s", snd_strerror(err));
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Set PCM parameters
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    
    err = snd_pcm_hw_params_any(pcm_handle, hw_params);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot initialize PCM parameters: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Set access type
    err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot set PCM access type: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Set sample format
    snd_pcm_format_t format;
    switch (current_config.bits_per_sample) {
        case 8:
            format = SND_PCM_FORMAT_S8;
            break;
        case 16:
            format = SND_PCM_FORMAT_S16_LE;
            break;
        case 24:
            format = SND_PCM_FORMAT_S24_LE;
            break;
        case 32:
            format = SND_PCM_FORMAT_S32_LE;
            break;
        default:
            format = SND_PCM_FORMAT_S16_LE;
            break;
    }
    
    err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, format);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot set PCM format: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Set sample rate
    unsigned int rate = current_config.sample_rate;
    err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot set PCM sample rate: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Set channels
    err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, current_config.channels);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot set PCM channels: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Set buffer size
    snd_pcm_uframes_t frames = buffer_size / (current_config.channels * current_config.bits_per_sample / 8);
    err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &frames);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot set PCM buffer size: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Apply parameters
    err = snd_pcm_hw_params(pcm_handle, hw_params);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot set PCM parameters: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Prepare PCM
    err = snd_pcm_prepare(pcm_handle);
    if (err < 0) {
        syslog(LOG_ERR, "Cannot prepare PCM: %s", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    is_running = true;
    
    pthread_mutex_unlock(&audio_mutex);
    
    syslog(LOG_INFO, "Audio output started");
    return 0;
}

int audio_output_stop(void) {
    pthread_mutex_lock(&audio_mutex);
    
    if (!is_running) {
        pthread_mutex_unlock(&audio_mutex);
        return 0;
    }
    
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
    
    is_running = false;
    
    pthread_mutex_unlock(&audio_mutex);
    
    syslog(LOG_INFO, "Audio output stopped");
    return 0;
}

int audio_output_write(const uint8_t *data, size_t length) {
    if (!data || length == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&audio_mutex);
    
    if (!is_running || !pcm_handle) {
        pthread_mutex_unlock(&audio_mutex);
        return -1;
    }
    
    // Write to ALSA
    snd_pcm_sframes_t frames_written = snd_pcm_writei(pcm_handle, data, 
                                                      length / (current_config.channels * current_config.bits_per_sample / 8));
    
    if (frames_written < 0) {
        // Handle underrun
        if (frames_written == -EPIPE) {
            syslog(LOG_WARNING, "PCM underrun occurred");
            snd_pcm_prepare(pcm_handle);
        } else {
            syslog(LOG_ERR, "PCM write error: %s", snd_strerror(frames_written));
            pthread_mutex_unlock(&audio_mutex);
            return -1;
        }
    }
    
    pthread_mutex_unlock(&audio_mutex);
    return 0;
}

int audio_output_set_volume(float volume) {
    if (volume < 0.0f || volume > 1.0f) {
        return -1;
    }
    
    pthread_mutex_lock(&audio_mutex);
    
    if (current_config.use_hw_volume && pcm_handle) {
        // Try to use hardware volume control
        snd_mixer_t *mixer;
        snd_mixer_elem_t *elem;
        snd_mixer_selem_id_t *sid;
        
        if (snd_mixer_open(&mixer, 0) == 0) {
            if (snd_mixer_attach(mixer, "default") == 0) {
                if (snd_mixer_selem_register(mixer, NULL, NULL) == 0) {
                    if (snd_mixer_load(mixer) == 0) {
                        snd_mixer_selem_id_alloca(&sid);
                        snd_mixer_selem_id_set_index(sid, 0);
                        snd_mixer_selem_id_set_name(sid, "Master");
                        
                        elem = snd_mixer_find_selem(mixer, sid);
                        if (elem) {
                            long min, max;
                            snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                            long vol = min + (long)((max - min) * volume);
                            snd_mixer_selem_set_playback_volume_all(elem, vol);
                        }
                    }
                }
            }
            snd_mixer_close(mixer);
        }
    }
    
    pthread_mutex_unlock(&audio_mutex);
    return 0;
}

float audio_output_get_volume(void) {
    pthread_mutex_lock(&audio_mutex);
    
    float volume = 0.5f; // Default volume
    
    if (current_config.use_hw_volume && pcm_handle) {
        snd_mixer_t *mixer;
        snd_mixer_elem_t *elem;
        snd_mixer_selem_id_t *sid;
        
        if (snd_mixer_open(&mixer, 0) == 0) {
            if (snd_mixer_attach(mixer, "default") == 0) {
                if (snd_mixer_selem_register(mixer, NULL, NULL) == 0) {
                    if (snd_mixer_load(mixer) == 0) {
                        snd_mixer_selem_id_alloca(&sid);
                        snd_mixer_selem_id_set_index(sid, 0);
                        snd_mixer_selem_id_set_name(sid, "Master");
                        
                        elem = snd_mixer_find_selem(mixer, sid);
                        if (elem) {
                            long min, max, vol;
                            snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                            snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
                            volume = (float)(vol - min) / (float)(max - min);
                        }
                    }
                }
            }
            snd_mixer_close(mixer);
        }
    }
    
    pthread_mutex_unlock(&audio_mutex);
    return volume;
}

bool audio_output_is_running(void) {
    pthread_mutex_lock(&audio_mutex);
    bool running = is_running;
    pthread_mutex_unlock(&audio_mutex);
    return running;
}

int audio_output_set_buffer_size(size_t size) {
    if (size < 1024 || size > 65536) {
        return -1;
    }
    
    pthread_mutex_lock(&audio_mutex);
    
    if (audio_buffer) {
        free(audio_buffer);
    }
    
    buffer_size = size;
    audio_buffer = malloc(buffer_size);
    if (!audio_buffer) {
        buffer_size = DEFAULT_BUFFER_SIZE;
        audio_buffer = malloc(buffer_size);
        if (!audio_buffer) {
            pthread_mutex_unlock(&audio_mutex);
            return -1;
        }
    }
    
    buffer_write_pos = 0;
    buffer_read_pos = 0;
    
    pthread_mutex_unlock(&audio_mutex);
    return 0;
}

size_t audio_output_get_buffer_size(void) {
    pthread_mutex_lock(&audio_mutex);
    size_t size = buffer_size;
    pthread_mutex_unlock(&audio_mutex);
    return size;
}

size_t audio_output_get_available_space(void) {
    pthread_mutex_lock(&audio_mutex);
    size_t available = buffer_size - ((buffer_write_pos - buffer_read_pos) % buffer_size);
    pthread_mutex_unlock(&audio_mutex);
    return available;
}
