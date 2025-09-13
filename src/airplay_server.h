#ifndef AIRPLAY_SERVER_H
#define AIRPLAY_SERVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct airplay_server airplay_server_t;

// AirPlay server configuration
typedef struct {
    char device_name[64];
    char model_name[32];
    char device_id[64];
    uint16_t port;
    bool enable_multiroom;
    char multiroom_group[32];
} airplay_config_t;

// Audio data callback
typedef void (*audio_data_callback_t)(const uint8_t *data, size_t length, 
                                     uint32_t sample_rate, uint8_t channels);

// Volume change callback
typedef void (*volume_callback_t)(float volume);

// Playback control callbacks
typedef void (*play_callback_t)(void);
typedef void (*pause_callback_t)(void);
typedef void (*stop_callback_t)(void);
typedef void (*next_callback_t)(void);
typedef void (*previous_callback_t)(void);

// AirPlay server functions
airplay_server_t* airplay_server_create(void);
int airplay_server_start(airplay_server_t *server);
int airplay_server_stop(airplay_server_t *server);
void airplay_server_destroy(airplay_server_t *server);
int airplay_server_process(airplay_server_t *server);

// Configuration functions
int airplay_server_set_config(airplay_server_t *server, const airplay_config_t *config);
int airplay_server_get_config(airplay_server_t *server, airplay_config_t *config);

// Callback registration
int airplay_server_set_audio_callback(airplay_server_t *server, audio_data_callback_t callback);
int airplay_server_set_volume_callback(airplay_server_t *server, volume_callback_t callback);
int airplay_server_set_playback_callbacks(airplay_server_t *server,
                                         play_callback_t play_cb,
                                         pause_callback_t pause_cb,
                                         stop_callback_t stop_cb,
                                         next_callback_t next_cb,
                                         previous_callback_t prev_cb);

// Status functions
bool airplay_server_is_connected(airplay_server_t *server);
const char* airplay_server_get_client_info(airplay_server_t *server);

#endif // AIRPLAY_SERVER_H
