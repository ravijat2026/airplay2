#include "network_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <syslog.h>

int network_get_local_ip(char *ip_buffer, size_t buffer_size) {
    if (!ip_buffer || buffer_size == 0) {
        return -1;
    }
    
    struct ifaddrs *ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        return -1;
    }
    
    for (ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        
        // Skip loopback and non-IPv4 interfaces
        if (ifa->ifa_addr->sa_family != AF_INET || 
            (ifa->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }
        
        // Skip interfaces that are down
        if (!(ifa->ifa_flags & IFF_UP)) {
            continue;
        }
        
        struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
        const char *ip = inet_ntoa(addr_in->sin_addr);
        
        // Skip private IPs that start with 127 (loopback)
        if (strncmp(ip, "127.", 4) == 0) {
            continue;
        }
        
        strncpy(ip_buffer, ip, buffer_size - 1);
        ip_buffer[buffer_size - 1] = '\0';
        
        freeifaddrs(ifaddrs_ptr);
        return 0;
    }
    
    freeifaddrs(ifaddrs_ptr);
    return -1;
}

int network_get_mac_address(char *mac_buffer, size_t buffer_size) {
    if (!mac_buffer || buffer_size < 18) { // MAC address is 17 chars + null terminator
        return -1;
    }
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    
    // Try common interface names
    const char *interfaces[] = {"eth0", "wlan0", "br0", "lan0", NULL};
    
    for (int i = 0; interfaces[i] != NULL; i++) {
        strncpy(ifr.ifr_name, interfaces[i], IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
            unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
            snprintf(mac_buffer, buffer_size, "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            close(sock);
            return 0;
        }
    }
    
    close(sock);
    return -1;
}

int network_is_port_available(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    return result == 0 ? 1 : 0; // 1 = available, 0 = not available
}

int network_create_udp_socket(uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int network_create_tcp_socket(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int network_send_udp_packet(int socket_fd, const char *ip, uint16_t port, 
                           const uint8_t *data, size_t length) {
    if (socket_fd < 0 || !ip || !data || length == 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        return -1;
    }
    
    ssize_t sent = sendto(socket_fd, data, length, 0, 
                         (struct sockaddr*)&addr, sizeof(addr));
    
    return sent == (ssize_t)length ? 0 : -1;
}

int network_receive_udp_packet(int socket_fd, char *ip_buffer, size_t ip_buffer_size,
                              uint16_t *port, uint8_t *data_buffer, size_t *length) {
    if (socket_fd < 0 || !ip_buffer || !port || !data_buffer || !length) {
        return -1;
    }
    
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    ssize_t received = recvfrom(socket_fd, data_buffer, *length, 0,
                               (struct sockaddr*)&addr, &addr_len);
    
    if (received < 0) {
        return -1;
    }
    
    *length = received;
    *port = ntohs(addr.sin_port);
    
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_buffer, ip_buffer_size) == NULL) {
        return -1;
    }
    
    return 0;
}

int network_close_socket(int socket_fd) {
    if (socket_fd >= 0) {
        return close(socket_fd);
    }
    return 0;
}

// Simplified mDNS functions (in a real implementation, you'd use Avahi or similar)
int mdns_register_service(const char *service_name, const char *service_type,
                         uint16_t port, const char *txt_record) {
    // This is a placeholder - in a real implementation, you'd use Avahi
    syslog(LOG_INFO, "mDNS service registered: %s.%s on port %d", 
           service_name, service_type, port);
    return 0;
}

int mdns_unregister_service(const char *service_name, const char *service_type) {
    // This is a placeholder - in a real implementation, you'd use Avahi
    syslog(LOG_INFO, "mDNS service unregistered: %s.%s", service_name, service_type);
    return 0;
}

int mdns_browse_services(const char *service_type, 
                        void (*callback)(const char *name, const char *ip, uint16_t port)) {
    // This is a placeholder - in a real implementation, you'd use Avahi
    syslog(LOG_INFO, "mDNS browsing for service: %s", service_type);
    return 0;
}

int airplay_create_discovery_socket(void) {
    return network_create_udp_socket(5353); // mDNS port
}

int airplay_send_discovery_response(int socket_fd, const char *client_ip, uint16_t client_port) {
    // Simplified AirPlay discovery response
    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/x-apple-plist+xml\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    
    return network_send_udp_packet(socket_fd, client_ip, client_port, 
                                  (const uint8_t*)response, strlen(response));
}

int airplay_handle_discovery_request(const uint8_t *request, size_t length,
                                    char *response, size_t *response_length) {
    if (!request || !response || !response_length) {
        return -1;
    }
    
    // Simple AirPlay discovery response
    const char *discovery_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/x-apple-plist+xml\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    
    size_t resp_len = strlen(discovery_response);
    if (*response_length < resp_len) {
        return -1;
    }
    
    strcpy(response, discovery_response);
    *response_length = resp_len;
    
    return 0;
}
