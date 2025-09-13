#ifndef MULTIROOM_H
#define MULTIROOM_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_ROOMS 8
#define MAX_ROOM_NAME_LEN 32

// Multiroom configuration
typedef struct {
    char room_name[MAX_ROOM_NAME_LEN];
    bool enabled;
    uint16_t port;
    char group_id[64];
} multiroom_config_t;

// Multiroom functions
int multiroom_init(void);
int multiroom_cleanup(void);
int multiroom_set_config(const multiroom_config_t *config);
int multiroom_get_config(multiroom_config_t *config);
int multiroom_start(void);
int multiroom_stop(void);
bool multiroom_is_enabled(void);
bool multiroom_is_running(void);

// Room management
int multiroom_add_room(const char *room_name, uint16_t port);
int multiroom_remove_room(const char *room_name);
int multiroom_get_room_count(void);
int multiroom_get_room_list(char room_names[MAX_ROOMS][MAX_ROOM_NAME_LEN]);

// Synchronization
int multiroom_sync_audio(const uint8_t *data, size_t length, uint32_t timestamp);
int multiroom_set_sync_delay(uint32_t delay_ms);
uint32_t multiroom_get_sync_delay(void);

// Callbacks
typedef void (*multiroom_room_added_callback_t)(const char *room_name);
typedef void (*multiroom_room_removed_callback_t)(const char *room_name);
typedef void (*multiroom_sync_callback_t)(const uint8_t *data, size_t length, uint32_t timestamp);

int multiroom_set_room_added_callback(multiroom_room_added_callback_t callback);
int multiroom_set_room_removed_callback(multiroom_room_removed_callback_t callback);
int multiroom_set_sync_callback(multiroom_sync_callback_t callback);

#endif // MULTIROOM_H
