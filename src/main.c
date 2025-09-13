#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <daemon.h>
#include "airplay_server.h"
#include "audio_output.h"
#include "volume_control.h"
#include "playback_control.h"
#include "multiroom.h"

static volatile int running = 1;
static airplay_server_t *server = NULL;

void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            syslog(LOG_INFO, "Received signal %d, shutting down...", sig);
            running = 0;
            break;
        default:
            break;
    }
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}

int main(int argc, char *argv[]) {
    int daemonize = 1;
    int opt;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "df")) != -1) {
        switch (opt) {
            case 'd':
                daemonize = 0;
                break;
            case 'f':
                daemonize = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d] [-f]\n", argv[0]);
                fprintf(stderr, "  -d: run in foreground\n");
                fprintf(stderr, "  -f: run as daemon\n");
                exit(EXIT_FAILURE);
        }
    }
    
    // Initialize logging
    if (daemonize) {
        openlog("airplay2-lite", LOG_PID | LOG_CONS, LOG_DAEMON);
        daemon(0, 0);
    } else {
        openlog("airplay2-lite", LOG_PID | LOG_CONS, LOG_PERROR);
    }
    
    syslog(LOG_INFO, "Starting AirPlay 2 Lite server...");
    
    // Setup signal handlers
    setup_signal_handlers();
    
    // Initialize audio output
    if (audio_output_init() != 0) {
        syslog(LOG_ERR, "Failed to initialize audio output");
        exit(EXIT_FAILURE);
    }
    
    // Initialize volume control
    if (volume_control_init() != 0) {
        syslog(LOG_ERR, "Failed to initialize volume control");
        audio_output_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Initialize playback control
    if (playback_control_init() != 0) {
        syslog(LOG_ERR, "Failed to initialize playback control");
        volume_control_cleanup();
        audio_output_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Initialize multiroom support
    if (multiroom_init() != 0) {
        syslog(LOG_ERR, "Failed to initialize multiroom support");
        playback_control_cleanup();
        volume_control_cleanup();
        audio_output_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Create and start AirPlay server
    server = airplay_server_create();
    if (!server) {
        syslog(LOG_ERR, "Failed to create AirPlay server");
        multiroom_cleanup();
        playback_control_cleanup();
        volume_control_cleanup();
        audio_output_cleanup();
        exit(EXIT_FAILURE);
    }
    
    if (airplay_server_start(server) != 0) {
        syslog(LOG_ERR, "Failed to start AirPlay server");
        airplay_server_destroy(server);
        multiroom_cleanup();
        playback_control_cleanup();
        volume_control_cleanup();
        audio_output_cleanup();
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_INFO, "AirPlay 2 Lite server started successfully");
    
    // Main loop
    while (running) {
        airplay_server_process(server);
        usleep(10000); // 10ms sleep to prevent excessive CPU usage
    }
    
    // Cleanup
    syslog(LOG_INFO, "Shutting down AirPlay 2 Lite server...");
    
    airplay_server_stop(server);
    airplay_server_destroy(server);
    multiroom_cleanup();
    playback_control_cleanup();
    volume_control_cleanup();
    audio_output_cleanup();
    
    syslog(LOG_INFO, "AirPlay 2 Lite server stopped");
    closelog();
    
    return EXIT_SUCCESS;
}
