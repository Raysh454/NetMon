#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 128

typedef enum {
    OS_AUTH = 0b100,
    OS_AUTH_INFO = 0b101,
    OS_INFORMER_INFO = 0b110,
    OS_UPDATE_USG = 0b111,
    OS_NOTIFY_INFORMER_TIMEOUT = 0b1000,
    OS_PING = 0b1001,
    OS_PONG = 0b1010,
} PTYPE;

inline uint64_t ntohll(uint64_t val) {
    return ((uint64_t)ntohl(val & 0xFFFFFFFF) << 32) | ntohl(val >> 32);
}

#endif
