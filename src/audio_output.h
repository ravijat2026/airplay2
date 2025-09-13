#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

// Audio output configuration
typedef struct {
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    const char *device_name;
    bool use_hw_volume;
} audio_config_t;

// Audio output functions
int audio_output_init(void);
int audio_output_cleanup(void);
int audio_output_configure(const audio_config_t *config);
int audio_output_start(void);
int audio_output_stop(void);
int audio_output_write(const uint8_t *data, size_t length);
int audio_output_set_volume(float volume);
float audio_output_get_volume(void);
bool audio_output_is_running(void);

// Buffer management
int audio_output_set_buffer_size(size_t size);
size_t audio_output_get_buffer_size(void);
size_t audio_output_get_available_space(void);

#endif // AUDIO_OUTPUT_H
