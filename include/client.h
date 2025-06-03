#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <process.h>
    #include <io.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close(s) closesocket(s)
    #define sleep(s) Sleep(s * 1000)
    typedef HANDLE pthread_t;
    typedef CRITICAL_SECTION pthread_mutex_t;
    #define pthread_mutex_init(m, a) InitializeCriticalSection(m)
    #define pthread_mutex_destroy(m) DeleteCriticalSection(m)
    #define pthread_mutex_lock(m) EnterCriticalSection(m)
    #define pthread_mutex_unlock(m) LeaveCriticalSection(m)
    #define pthread_create(t, a, f, d) (*t = (HANDLE)_beginthreadex(NULL, 0, f, d, 0, NULL))
    #define pthread_join(t, r) WaitForSingleObject(t, INFINITE)
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Client-specific constants
#define MAX_USERNAME 32
#define MAX_MESSAGES 1000
#define MAX_USERS 50
#define MAX_CONVERSATIONS 20
#define MAX_PASSWORD 64
#define BUFFER_SIZE 1024

// Message structure
typedef struct {
    char content[BUFFER_SIZE];
    char username[MAX_USERNAME];
    char recipient[MAX_USERNAME];
    time_t timestamp;
    int is_private;
} message_t;

// User structure
typedef struct {
    char username[MAX_USERNAME];
    int active;
    int unread_count;
    time_t last_message_time;
} user_t;

// Conversation structure
typedef struct {
    char username[MAX_USERNAME];
    message_t messages[MAX_MESSAGES];
    int message_count;
    int unread_count;
    time_t last_message_time;
} conversation_t;

// Global variables (declared in main.c)
extern message_t messages[MAX_MESSAGES];
extern user_t users[MAX_USERS];
extern conversation_t conversations[MAX_CONVERSATIONS];
extern int message_count;
extern int user_count;
extern int conversation_count;
extern pthread_mutex_t display_mutex;
extern int sockfd;
extern char my_username[MAX_USERNAME];
extern char current_recipient[MAX_USERNAME];

// Error handling
void error(const char *msg);

// Message handling
void print_message(const char *sender, const char *content, int is_system, const char *recipient);
void print_prompt();
void print_help();
void *receive_messages(void *arg);
void update_unread_count(const char *sender, int increment);

// Conversation management
void add_to_conversation(const char *sender, const char *content, const char *recipient, int is_private);
void print_active_conversations();
void load_conversation_history(const char *target, const char *history);
void print_conversation_history(const char *target);
void update_user_list(const char *user_list);

// Display functions
void print_welcome();
void print_disconnect();
void print_error(const char *msg);
void print_system(const char *msg);
void print_user_joined(const char *username);
void print_user_left(const char *username);
void print_private_start(const char *username);
void print_private_end(const char *username);
void print_no_such_user(const char *username);
void print_no_history(const char *username);
void print_invalid_command();
void print_usage();
void print_version();
void print_credits();

// Debug functions
void print_debug(const char *msg);
void print_debug_hex(const char *prefix, const unsigned char *data, size_t len);
void print_debug_str(const char *prefix, const char *str);
void print_debug_int(const char *prefix, int value);
void print_debug_time(const char *prefix, time_t t);
void print_debug_user(const char *prefix, const user_t *user);
void print_debug_message(const char *prefix, const message_t *msg);
void print_debug_conversation(const char *prefix, const conversation_t *conv);

// Cleanup
void cleanup();

#endif // CLIENT_H 