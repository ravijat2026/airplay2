#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <stdint.h>
#include <stdbool.h>

// Network utility functions
int network_get_local_ip(char *ip_buffer, size_t buffer_size);
int network_get_mac_address(char *mac_buffer, size_t buffer_size);
int network_is_port_available(uint16_t port);
int network_create_udp_socket(uint16_t port);
int network_create_tcp_socket(uint16_t port);
int network_send_udp_packet(int socket_fd, const char *ip, uint16_t port, 
                           const uint8_t *data, size_t length);
int network_receive_udp_packet(int socket_fd, char *ip_buffer, size_t ip_buffer_size,
                              uint16_t *port, uint8_t *data_buffer, size_t *length);
int network_close_socket(int socket_fd);

// mDNS/Bonjour utilities
int mdns_register_service(const char *service_name, const char *service_type,
                         uint16_t port, const char *txt_record);
int mdns_unregister_service(const char *service_name, const char *service_type);
int mdns_browse_services(const char *service_type, 
                        void (*callback)(const char *name, const char *ip, uint16_t port));

// AirPlay specific network functions
int airplay_create_discovery_socket(void);
int airplay_send_discovery_response(int socket_fd, const char *client_ip, uint16_t client_port);
int airplay_handle_discovery_request(const uint8_t *request, size_t length,
                                    char *response, size_t *response_length);

#endif // NETWORK_UTILS_H
