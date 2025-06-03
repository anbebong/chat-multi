#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "server.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#define SERVER_PORT 56001

// Global variable definitions (declared as extern in server.h)
client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
server_conversation_t conversations[MAX_CONVERSATIONS];
int conversation_count = 0;
pthread_mutex_t conversations_mutex = PTHREAD_MUTEX_INITIALIZER;

void cleanup() {
    // Close all client sockets
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].sockfd);
            clients[i].active = 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    // Destroy mutexes
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&conversations_mutex);
}

int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    struct sockaddr_in server_addr;
    int server_fd;
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creating socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Error listening on socket");
        close(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    printf("Server started on port %d\n", SERVER_PORT);
    
    // Initialize mutexes
    pthread_mutex_init(&clients_mutex, NULL);
    pthread_mutex_init(&conversations_mutex, NULL);
    
    // Load saved conversations
    load_conversations();
    
    // Register cleanup handler
    atexit(cleanup);
    
    // Main server loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Accept new connection
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Error accepting connection");
            continue;
        }
        
        // Create new client structure
        client_t *client = NULL;
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                client = &clients[i];
                client->sockfd = client_fd;
                client->active = 0;  // Will be set to 1 after username is received
                pthread_mutex_init(&client->mutex, NULL);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        
        if (client == NULL) {
            close(client_fd);
            continue;
        }
        
        // Create thread for new client
        if (pthread_create(&client->thread, NULL, handle_client, client) != 0) {
            perror("Error creating thread");
            close(client_fd);
            continue;
        }
    }
    
    // Cleanup
    close(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
} 
