#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <map>
#include <mutex>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <random>

#define PORT 8080
#define BUFFER_SIZE 128
#define UPDATE_OVERSEERS_INTERVAL 5
#define INFORMER_TIMEOUT 10
#define OVERSEER_TIMEOUT 10
#define CLEANUP_INFORMERS_INTERVAL 20
#define CLEANUP_OVERSEERS_INTERVAL 20

typedef enum {
    INF_INIT = 0b00,
    INF_APPROVE = 0b01,
    INF_USAGE = 0b10,
    INF_ACK = 0b11,
    OS_AUTH = 0b100,
    OS_AUTH_INFO = 0b101,
    OS_INFORMER_INFO = 0b110,
    OS_UPDATE_USG = 0b111,
    OS_NOTIFY_INFORMER_TIMEOUT = 0b1000,
    OS_PING = 0b1001,
    OS_PONG = 0b1010,
} PTYPE;
