#ifndef SERVER_H
#define SERVER_H

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
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Server-specific constants
#define MAX_CLIENTS 10
#define MAX_USERNAME 32
#define MAX_PASSWORD 64
#define MAX_MESSAGES 1000
#define MAX_CONVERSATIONS 100
#define MAX_MESSAGES_PER_CONVERSATION 100
#define CONVERSATIONS_FILE "conversations.txt"
#define USERS_FILE "users.txt"
#define BUFFER_SIZE 1024

// Client structure
typedef struct {
    int sockfd;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int active;
    pthread_t thread;
    pthread_mutex_t mutex;
} client_t;

// Server message structure
typedef struct {
    char sender[MAX_USERNAME];
    char recipient[MAX_USERNAME];
    char content[BUFFER_SIZE];
    char timestamp[20];
    int is_private;
} server_message_t;

// Server conversation structure
typedef struct {
    char participant1[MAX_USERNAME];
    char participant2[MAX_USERNAME];
    server_message_t messages[MAX_MESSAGES_PER_CONVERSATION];
    int message_count;
    time_t last_active;
} server_conversation_t;

// Global variables (declared in main.c)
extern client_t clients[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t clients_mutex;
extern server_conversation_t conversations[MAX_CONVERSATIONS];
extern int conversation_count;
extern pthread_mutex_t conversations_mutex;

// Error handling
void error(const char *msg);

// Message handling
void broadcast_message(const char *sender, const char *content, const char *recipient);
void send_user_list(int client_sock);
void send_conversation_history(int client_sock, const char *target, const char *username);

// Client handling
void *handle_client(void *arg);
void add_client(int sockfd, const char *username, const char *password);
void remove_client(int sockfd);
int find_client(const char *username);
void notify_user_joined(const char *username);
void notify_user_left(const char *username);
void broadcast_message_to_socket(const char *message, int sender_sock, const char *recipient);

// Storage management
void save_conversation(const server_message_t *msg);
void load_conversations();
void save_conversations();
void load_users();
void save_users();

// Debug functions
void print_debug(const char *msg);
void print_debug_hex(const char *prefix, const unsigned char *data, size_t len);
void print_debug_str(const char *prefix, const char *str);
void print_debug_int(const char *prefix, int value);
void print_debug_time(const char *prefix, time_t t);
void print_debug_client(const char *prefix, const client_t *client);
void print_debug_message(const char *prefix, const server_message_t *msg);
void print_debug_conversation(const char *prefix, const server_conversation_t *conv);

// Cleanup
void cleanup();

int user_exists(const char *username);
int check_user_password(const char *username, const char *password);
int is_user_online(const char *username);
void register_user(const char *username, const char *password);

#endif // SERVER_H 