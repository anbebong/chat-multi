#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

// Network settings
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Message types
#define MSG_PUBLIC 0
#define MSG_PRIVATE 1
#define MSG_SYSTEM 2

// Command types
#define CMD_USERS 0
#define CMD_HISTORY 1
#define CMD_PRIVATE 2
#define CMD_PUBLIC 3
#define CMD_QUIT 4
#define CMD_HELP 5
#define CMD_CLEAR 6
#define CMD_CONV 7

// Error codes
#define ERR_NONE 0
#define ERR_SOCKET 1
#define ERR_CONNECT 2
#define ERR_SEND 3
#define ERR_RECV 4
#define ERR_BIND 5
#define ERR_LISTEN 6
#define ERR_ACCEPT 7
#define ERR_THREAD 8
#define ERR_MEMORY 9
#define ERR_INVALID 10

// Debug settings
#ifdef DEBUG
#define DEBUG_PRINT(msg) printf("[DEBUG] %s\n", msg)
#else
#define DEBUG_PRINT(msg)
#endif

// Helper macros
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Helper functions
static inline void error(const char *msg) {
    perror(msg);
    exit(1);
}

static inline void debug_print(const char *msg) {
#ifdef DEBUG
    printf("[DEBUG] %s\n", msg);
#endif
}

static inline void debug_print_hex(const char *prefix, const unsigned char *data, size_t len) {
#ifdef DEBUG
    printf("[DEBUG] %s: ", prefix);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
#endif
}

static inline void debug_print_str(const char *prefix, const char *str) {
#ifdef DEBUG
    printf("[DEBUG] %s: %s\n", prefix, str);
#endif
}

static inline void debug_print_int(const char *prefix, int value) {
#ifdef DEBUG
    printf("[DEBUG] %s: %d\n", prefix, value);
#endif
}

static inline void debug_print_time(const char *prefix, time_t t) {
#ifdef DEBUG
    char time_str[20];
    struct tm *tm = localtime(&t);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    printf("[DEBUG] %s: %s\n", prefix, time_str);
#endif
}

#endif // COMMON_H 