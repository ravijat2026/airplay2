#include "airplay_server.h"
#include "crypto_utils.h"
#include "network_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/thread-watch.h>
#include <errno.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#define AIRPLAY_PORT 7000
#define MAX_CLIENTS 4
#define BUFFER_SIZE 4096

struct airplay_server {
    int socket_fd;
    struct sockaddr_in server_addr;
    airplay_config_t config;
    
    // Callbacks
    audio_data_callback_t audio_callback;
    volume_callback_t volume_callback;
    play_callback_t play_callback;
    pause_callback_t pause_callback;
    stop_callback_t stop_callback;
    next_callback_t next_callback;
    previous_callback_t previous_callback;
    
    // Client connections
    struct {
        int fd;
        struct sockaddr_in addr;
        bool connected;
        char session_id[64];
    } clients[MAX_CLIENTS];
    
    // Avahi for mDNS
    AvahiClient *avahi_client;
    AvahiEntryGroup *entry_group;
    
    // Running state
    bool running;
};

static void avahi_client_callback(AvahiClient *c, AvahiClientState state, void *userdata);
static void avahi_entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata);
static void* server_thread(void *arg);
static int handle_client_request(airplay_server_t *server, int client_fd);
static int parse_airplay_request(const char *request, char *method, char *path, char *headers);
static int handle_rtsp_request(airplay_server_t *server, int client_fd, const char *request);
static int handle_http_request(airplay_server_t *server, int client_fd, const char *request);

airplay_server_t* airplay_server_create(void) {
    airplay_server_t *server = calloc(1, sizeof(airplay_server_t));
    if (!server) {
        return NULL;
    }
    
    // Set default configuration
    strncpy(server->config.device_name, "OpenWRT AirPlay", sizeof(server->config.device_name) - 1);
    strncpy(server->config.model_name, "OpenWRT", sizeof(server->config.model_name) - 1);
    strncpy(server->config.device_id, "OpenWRT-AirPlay-001", sizeof(server->config.device_id) - 1);
    server->config.port = AIRPLAY_PORT;
    server->config.enable_multiroom = false;
    
    return server;
}

int airplay_server_start(airplay_server_t *server) {
    if (!server) {
        return -1;
    }
    
    // Create socket
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        syslog(LOG_ERR, "Failed to create socket");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    memset(&server->server_addr, 0, sizeof(server->server_addr));
    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port = htons(server->config.port);
    
    if (bind(server->socket_fd, (struct sockaddr*)&server->server_addr, 
             sizeof(server->server_addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket to port %d", server->config.port);
        close(server->socket_fd);
        return -1;
    }
    
    // Listen for connections
    if (listen(server->socket_fd, MAX_CLIENTS) < 0) {
        syslog(LOG_ERR, "Failed to listen on socket");
        close(server->socket_fd);
        return -1;
    }
    
    // Initialize Avahi client for mDNS
    int error;
    AvahiThreadedPoll *poll = avahi_threaded_poll_new();
    if (!poll) {
        syslog(LOG_ERR, "Failed to create Avahi threaded poll");
        close(server->socket_fd);
        return -1;
    }
    
    server->avahi_client = avahi_client_new(avahi_threaded_poll_get(poll), 
                                           AVAHI_CLIENT_NO_FAIL, 
                                           avahi_client_callback, 
                                           server, &error);
    if (!server->avahi_client) {
        syslog(LOG_ERR, "Failed to create Avahi client: %s", avahi_strerror(error));
        close(server->socket_fd);
        return -1;
    }
    
    server->running = true;
    syslog(LOG_INFO, "AirPlay server started on port %d", server->config.port);
    
    return 0;
}

int airplay_server_stop(airplay_server_t *server) {
    if (!server) {
        return -1;
    }
    
    server->running = false;
    
    // Close client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            close(server->clients[i].fd);
            server->clients[i].connected = false;
        }
    }
    
    // Close server socket
    if (server->socket_fd >= 0) {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
    
    // Cleanup Avahi
    if (server->entry_group) {
        avahi_entry_group_free(server->entry_group);
        server->entry_group = NULL;
    }
    
    if (server->avahi_client) {
        avahi_client_free(server->avahi_client);
        server->avahi_client = NULL;
    }
    
    syslog(LOG_INFO, "AirPlay server stopped");
    return 0;
}

void airplay_server_destroy(airplay_server_t *server) {
    if (server) {
        airplay_server_stop(server);
        free(server);
    }
}

int airplay_server_process(airplay_server_t *server) {
    if (!server || !server->running) {
        return -1;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    int max_fd = server->socket_fd;
    
    FD_ZERO(&read_fds);
    FD_SET(server->socket_fd, &read_fds);
    
    // Add client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            FD_SET(server->clients[i].fd, &read_fds);
            if (server->clients[i].fd > max_fd) {
                max_fd = server->clients[i].fd;
            }
        }
    }
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms
    
    int result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result < 0) {
        if (errno == EINTR) {
            return 0; // Interrupted by signal
        }
        syslog(LOG_ERR, "select() failed: %s", strerror(errno));
        return -1;
    }
    
    if (result == 0) {
        return 0; // Timeout
    }
    
    // Check for new connections
    if (FD_ISSET(server->socket_fd, &read_fds)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server->socket_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) {
            // Find free client slot
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!server->clients[i].connected) {
                    server->clients[i].fd = client_fd;
                    server->clients[i].addr = client_addr;
                    server->clients[i].connected = true;
                    memset(server->clients[i].session_id, 0, sizeof(server->clients[i].session_id));
                    
                    syslog(LOG_INFO, "New client connected from %s:%d", 
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    break;
                }
            }
            
            // If no free slot, close connection
            if (client_fd >= 0) {
                bool found_slot = false;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (server->clients[i].fd == client_fd) {
                        found_slot = true;
                        break;
                    }
                }
                if (!found_slot) {
                    close(client_fd);
                    syslog(LOG_WARNING, "No free client slots, connection rejected");
                }
            }
        }
    }
    
    // Handle client requests
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected && FD_ISSET(server->clients[i].fd, &read_fds)) {
            if (handle_client_request(server, server->clients[i].fd) < 0) {
                // Client disconnected or error
                syslog(LOG_INFO, "Client disconnected");
                close(server->clients[i].fd);
                server->clients[i].connected = false;
            }
        }
    }
    
    return 0;
}

static int handle_client_request(airplay_server_t *server, int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    
    if (bytes_read <= 0) {
        return -1; // Client disconnected or error
    }
    
    buffer[bytes_read] = '\0';
    
    // Parse request type and route accordingly
    if (strncmp(buffer, "OPTIONS", 7) == 0 || 
        strncmp(buffer, "POST", 4) == 0 ||
        strncmp(buffer, "GET", 3) == 0) {
        return handle_http_request(server, client_fd, buffer);
    } else if (strncmp(buffer, "ANNOUNCE", 8) == 0 ||
               strncmp(buffer, "SETUP", 5) == 0 ||
               strncmp(buffer, "RECORD", 6) == 0 ||
               strncmp(buffer, "PAUSE", 5) == 0 ||
               strncmp(buffer, "FLUSH", 5) == 0 ||
               strncmp(buffer, "TEARDOWN", 8) == 0) {
        return handle_rtsp_request(server, client_fd, buffer);
    }
    
    return 0;
}

static int handle_http_request(airplay_server_t *server, int client_fd, const char *request) {
    // Simple HTTP response for AirPlay discovery
    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/x-apple-plist+xml\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    
    send(client_fd, response, strlen(response), 0);
    return 0;
}

static int handle_rtsp_request(airplay_server_t *server, int client_fd, const char *request) {
    // Handle RTSP requests for audio streaming
    char response[1024];
    
    if (strncmp(request, "ANNOUNCE", 8) == 0) {
        snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: 1\r\n"
            "Server: AirPlay/220.68\r\n"
            "\r\n");
    } else if (strncmp(request, "SETUP", 5) == 0) {
        snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: 2\r\n"
            "Server: AirPlay/220.68\r\n"
            "Transport: RTP/AVP/UDP;unicast;interleaved=0-1\r\n"
            "\r\n");
    } else if (strncmp(request, "RECORD", 6) == 0) {
        snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: 3\r\n"
            "Server: AirPlay/220.68\r\n"
            "\r\n");
    } else {
        snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: 1\r\n"
            "Server: AirPlay/220.68\r\n"
            "\r\n");
    }
    
    send(client_fd, response, strlen(response), 0);
    return 0;
}

// Avahi callbacks
static void avahi_client_callback(AvahiClient *c, AvahiClientState state, void *userdata) {
    airplay_server_t *server = (airplay_server_t*)userdata;
    
    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:
            if (!server->entry_group) {
                server->entry_group = avahi_entry_group_new(c, avahi_entry_group_callback, server);
                if (server->entry_group) {
                    avahi_entry_group_add_service(server->entry_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0,
                                                 server->config.device_name, "_airplay._tcp", NULL, NULL,
                                                 server->config.port, NULL);
                    avahi_entry_group_commit(server->entry_group);
                }
            }
            break;
        case AVAHI_CLIENT_FAILURE:
            syslog(LOG_ERR, "Avahi client failure");
            break;
        default:
            break;
    }
}

static void avahi_entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata) {
    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
            syslog(LOG_INFO, "AirPlay service registered with mDNS");
            break;
        case AVAHI_ENTRY_GROUP_COLLISION:
            syslog(LOG_WARNING, "AirPlay service name collision");
            break;
        case AVAHI_ENTRY_GROUP_FAILURE:
            syslog(LOG_ERR, "Failed to register AirPlay service");
            break;
        default:
            break;
    }
}

// Configuration functions
int airplay_server_set_config(airplay_server_t *server, const airplay_config_t *config) {
    if (!server || !config) {
        return -1;
    }
    
    server->config = *config;
    return 0;
}

int airplay_server_get_config(airplay_server_t *server, airplay_config_t *config) {
    if (!server || !config) {
        return -1;
    }
    
    *config = server->config;
    return 0;
}

// Callback registration functions
int airplay_server_set_audio_callback(airplay_server_t *server, audio_data_callback_t callback) {
    if (!server) {
        return -1;
    }
    
    server->audio_callback = callback;
    return 0;
}

int airplay_server_set_volume_callback(airplay_server_t *server, volume_callback_t callback) {
    if (!server) {
        return -1;
    }
    
    server->volume_callback = callback;
    return 0;
}

int airplay_server_set_playback_callbacks(airplay_server_t *server,
                                         play_callback_t play_cb,
                                         pause_callback_t pause_cb,
                                         stop_callback_t stop_cb,
                                         next_callback_t next_cb,
                                         previous_callback_t prev_cb) {
    if (!server) {
        return -1;
    }
    
    server->play_callback = play_cb;
    server->pause_callback = pause_cb;
    server->stop_callback = stop_cb;
    server->next_callback = next_cb;
    server->previous_callback = prev_cb;
    return 0;
}

// Status functions
bool airplay_server_is_connected(airplay_server_t *server) {
    if (!server) {
        return false;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            return true;
        }
    }
    return false;
}

const char* airplay_server_get_client_info(airplay_server_t *server) {
    if (!server) {
        return NULL;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            static char client_info[64];
            snprintf(client_info, sizeof(client_info), "%s:%d",
                    inet_ntoa(server->clients[i].addr.sin_addr),
                    ntohs(server->clients[i].addr.sin_port));
            return client_info;
        }
    }
    return NULL;
}
