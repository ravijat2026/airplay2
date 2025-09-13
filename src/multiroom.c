#include "multiroom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static pthread_mutex_t multiroom_mutex = PTHREAD_MUTEX_INITIALIZER;
static multiroom_config_t config;
static bool is_running = false;
static bool is_enabled = false;

// Room management
static char rooms[MAX_ROOMS][MAX_ROOM_NAME_LEN];
static int room_count = 0;
static int room_sockets[MAX_ROOMS];
static struct sockaddr_in room_addresses[MAX_ROOMS];

// Callbacks
static multiroom_room_added_callback_t room_added_callback = NULL;
static multiroom_room_removed_callback_t room_removed_callback = NULL;
static multiroom_sync_callback_t sync_callback = NULL;

// Sync settings
static uint32_t sync_delay_ms = 100;

int multiroom_init(void) {
    pthread_mutex_lock(&multiroom_mutex);
    
    // Set default configuration
    strncpy(config.room_name, "Default Room", sizeof(config.room_name) - 1);
    config.enabled = false;
    config.port = 7001;
    strncpy(config.group_id, "default-group", sizeof(config.group_id) - 1);
    
    // Initialize room data
    room_count = 0;
    for (int i = 0; i < MAX_ROOMS; i++) {
        memset(rooms[i], 0, sizeof(rooms[i]));
        room_sockets[i] = -1;
        memset(&room_addresses[i], 0, sizeof(room_addresses[i]));
    }
    
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Multiroom initialized");
    return 0;
}

int multiroom_cleanup(void) {
    pthread_mutex_lock(&multiroom_mutex);
    
    if (is_running) {
        multiroom_stop();
    }
    
    // Close all room sockets
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (room_sockets[i] >= 0) {
            close(room_sockets[i]);
            room_sockets[i] = -1;
        }
    }
    
    // Clear callbacks
    room_added_callback = NULL;
    room_removed_callback = NULL;
    sync_callback = NULL;
    
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Multiroom cleaned up");
    return 0;
}

int multiroom_set_config(const multiroom_config_t *new_config) {
    if (!new_config) {
        return -1;
    }
    
    pthread_mutex_lock(&multiroom_mutex);
    config = *new_config;
    is_enabled = config.enabled;
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Multiroom config updated: %s, enabled=%d", 
           config.room_name, config.enabled);
    return 0;
}

int multiroom_get_config(multiroom_config_t *out_config) {
    if (!out_config) {
        return -1;
    }
    
    pthread_mutex_lock(&multiroom_mutex);
    *out_config = config;
    pthread_mutex_unlock(&multiroom_mutex);
    return 0;
}

int multiroom_start(void) {
    pthread_mutex_lock(&multiroom_mutex);
    
    if (!is_enabled) {
        pthread_mutex_unlock(&multiroom_mutex);
        return 0; // Not enabled, but not an error
    }
    
    if (is_running) {
        pthread_mutex_unlock(&multiroom_mutex);
        return 0; // Already running
    }
    
    // Create UDP socket for multiroom communication
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        syslog(LOG_ERR, "Failed to create multiroom socket");
        pthread_mutex_unlock(&multiroom_mutex);
        return -1;
    }
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config.port);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind multiroom socket to port %d", config.port);
        close(sock);
        pthread_mutex_unlock(&multiroom_mutex);
        return -1;
    }
    
    is_running = true;
    
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Multiroom started on port %d", config.port);
    return 0;
}

int multiroom_stop(void) {
    pthread_mutex_lock(&multiroom_mutex);
    
    if (!is_running) {
        pthread_mutex_unlock(&multiroom_mutex);
        return 0;
    }
    
    // Close all room sockets
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (room_sockets[i] >= 0) {
            close(room_sockets[i]);
            room_sockets[i] = -1;
        }
    }
    
    is_running = false;
    
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Multiroom stopped");
    return 0;
}

bool multiroom_is_enabled(void) {
    pthread_mutex_lock(&multiroom_mutex);
    bool enabled = is_enabled;
    pthread_mutex_unlock(&multiroom_mutex);
    return enabled;
}

bool multiroom_is_running(void) {
    pthread_mutex_lock(&multiroom_mutex);
    bool running = is_running;
    pthread_mutex_unlock(&multiroom_mutex);
    return running;
}

int multiroom_add_room(const char *room_name, uint16_t port) {
    if (!room_name || strlen(room_name) == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&multiroom_mutex);
    
    if (room_count >= MAX_ROOMS) {
        syslog(LOG_WARNING, "Maximum number of rooms reached");
        pthread_mutex_unlock(&multiroom_mutex);
        return -1;
    }
    
    // Check if room already exists
    for (int i = 0; i < room_count; i++) {
        if (strcmp(rooms[i], room_name) == 0) {
            syslog(LOG_WARNING, "Room '%s' already exists", room_name);
            pthread_mutex_unlock(&multiroom_mutex);
            return -1;
        }
    }
    
    // Add new room
    strncpy(rooms[room_count], room_name, sizeof(rooms[room_count]) - 1);
    rooms[room_count][sizeof(rooms[room_count]) - 1] = '\0';
    
    // Create socket for this room
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        room_sockets[room_count] = sock;
        
        // Set up address for this room (simplified - in real implementation,
        // you'd need to discover room addresses via mDNS or configuration)
        memset(&room_addresses[room_count], 0, sizeof(room_addresses[room_count]));
        room_addresses[room_count].sin_family = AF_INET;
        room_addresses[room_count].sin_port = htons(port);
        // room_addresses[room_count].sin_addr.s_addr = inet_addr("192.168.1.100"); // Example
    }
    
    room_count++;
    
    // Notify callback
    if (room_added_callback) {
        room_added_callback(room_name);
    }
    
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Added room '%s' on port %d", room_name, port);
    return 0;
}

int multiroom_remove_room(const char *room_name) {
    if (!room_name) {
        return -1;
    }
    
    pthread_mutex_lock(&multiroom_mutex);
    
    int room_index = -1;
    for (int i = 0; i < room_count; i++) {
        if (strcmp(rooms[i], room_name) == 0) {
            room_index = i;
            break;
        }
    }
    
    if (room_index == -1) {
        syslog(LOG_WARNING, "Room '%s' not found", room_name);
        pthread_mutex_unlock(&multiroom_mutex);
        return -1;
    }
    
    // Close socket
    if (room_sockets[room_index] >= 0) {
        close(room_sockets[room_index]);
    }
    
    // Shift remaining rooms
    for (int i = room_index; i < room_count - 1; i++) {
        strcpy(rooms[i], rooms[i + 1]);
        room_sockets[i] = room_sockets[i + 1];
        room_addresses[i] = room_addresses[i + 1];
    }
    
    room_count--;
    
    // Clear last room data
    memset(rooms[room_count], 0, sizeof(rooms[room_count]));
    room_sockets[room_count] = -1;
    memset(&room_addresses[room_count], 0, sizeof(room_addresses[room_count]));
    
    // Notify callback
    if (room_removed_callback) {
        room_removed_callback(room_name);
    }
    
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Removed room '%s'", room_name);
    return 0;
}

int multiroom_get_room_count(void) {
    pthread_mutex_lock(&multiroom_mutex);
    int count = room_count;
    pthread_mutex_unlock(&multiroom_mutex);
    return count;
}

int multiroom_get_room_list(char room_names[MAX_ROOMS][MAX_ROOM_NAME_LEN]) {
    if (!room_names) {
        return -1;
    }
    
    pthread_mutex_lock(&multiroom_mutex);
    
    for (int i = 0; i < room_count; i++) {
        strncpy(room_names[i], rooms[i], MAX_ROOM_NAME_LEN - 1);
        room_names[i][MAX_ROOM_NAME_LEN - 1] = '\0';
    }
    
    pthread_mutex_unlock(&multiroom_mutex);
    return room_count;
}

int multiroom_sync_audio(const uint8_t *data, size_t length, uint32_t timestamp) {
    if (!data || length == 0 || !is_running) {
        return -1;
    }
    
    pthread_mutex_lock(&multiroom_mutex);
    
    // Send audio data to all rooms
    for (int i = 0; i < room_count; i++) {
        if (room_sockets[i] >= 0) {
            // In a real implementation, you'd send the audio data with timestamp
            // for synchronization. This is a simplified version.
            sendto(room_sockets[i], data, length, 0,
                   (struct sockaddr*)&room_addresses[i], sizeof(room_addresses[i]));
        }
    }
    
    // Notify sync callback
    if (sync_callback) {
        sync_callback(data, length, timestamp);
    }
    
    pthread_mutex_unlock(&multiroom_mutex);
    return 0;
}

int multiroom_set_sync_delay(uint32_t delay_ms) {
    pthread_mutex_lock(&multiroom_mutex);
    sync_delay_ms = delay_ms;
    pthread_mutex_unlock(&multiroom_mutex);
    
    syslog(LOG_INFO, "Multiroom sync delay set to %d ms", delay_ms);
    return 0;
}

uint32_t multiroom_get_sync_delay(void) {
    pthread_mutex_lock(&multiroom_mutex);
    uint32_t delay = sync_delay_ms;
    pthread_mutex_unlock(&multiroom_mutex);
    return delay;
}

int multiroom_set_room_added_callback(multiroom_room_added_callback_t callback) {
    pthread_mutex_lock(&multiroom_mutex);
    room_added_callback = callback;
    pthread_mutex_unlock(&multiroom_mutex);
    return 0;
}

int multiroom_set_room_removed_callback(multiroom_room_removed_callback_t callback) {
    pthread_mutex_lock(&multiroom_mutex);
    room_removed_callback = callback;
    pthread_mutex_unlock(&multiroom_mutex);
    return 0;
}

int multiroom_set_sync_callback(multiroom_sync_callback_t callback) {
    pthread_mutex_lock(&multiroom_mutex);
    sync_callback = callback;
    pthread_mutex_unlock(&multiroom_mutex);
    return 0;
}
