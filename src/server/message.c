#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "server.h"

extern client_t clients[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t clients_mutex;
extern server_conversation_t conversations[MAX_CONVERSATIONS];
extern int conversation_count;
extern pthread_mutex_t conversations_mutex;

// Message handling functions
void broadcast_message_to_socket(const char *message, int sender_socket, const char *recipient) {
    pthread_mutex_lock(&clients_mutex);
    
    // Find sender's username
    char sender_username[MAX_USERNAME] = "";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd == sender_socket) {
            strncpy(sender_username, clients[i].username, MAX_USERNAME - 1);
            break;
        }
    }
    
    printf("Debug: Broadcasting message from %s to %s\n", sender_username, recipient);
    
    // Send to all clients except sender
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd != sender_socket) {
            // For private messages, only send to the recipient
            if (strcmp(recipient, "all") != 0) {
                if (strcmp(clients[i].username, recipient) == 0) {
                    printf("Debug: Sending private message to %s\n", clients[i].username);
                    if (send(clients[i].sockfd, message, strlen(message), 0) < 0) {
                        perror("Failed to send private message");
                    }
                }
            } else {
                // For public messages, send to everyone except sender
                printf("Debug: Sending public message to %s\n", clients[i].username);
                if (send(clients[i].sockfd, message, strlen(message), 0) < 0) {
                    perror("Failed to send message");
                }
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(const char *sender, const char *content, const char *recipient) {
    pthread_mutex_lock(&clients_mutex);
    
    // Create message for storage
    server_message_t msg;
    strncpy(msg.sender, sender, MAX_USERNAME - 1);
    strncpy(msg.recipient, recipient, MAX_USERNAME - 1);
    strncpy(msg.content, content, BUFFER_SIZE - 1);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(msg.timestamp, sizeof(msg.timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    msg.is_private = (strcmp(recipient, "all") != 0);
    
    // Find or create conversation for storage
    server_conversation_t *conv = NULL;
    const char *storage_username = (strcmp(recipient, "all") == 0) ? "all" : recipient;
    
    for (int i = 0; i < conversation_count; i++) {
        if (strcmp(conversations[i].participant1, storage_username) == 0 ||
            strcmp(conversations[i].participant2, storage_username) == 0) {
            conv = &conversations[i];
            break;
        }
    }
    
    if (conv == NULL && conversation_count < MAX_CONVERSATIONS) {
        conv = &conversations[conversation_count++];
        strncpy(conv->participant1, storage_username, MAX_USERNAME - 1);
        conv->message_count = 0;
        conv->last_active = time(NULL);
    }
    
    if (conv != NULL && conv->message_count < MAX_MESSAGES_PER_CONVERSATION) {
        conv->messages[conv->message_count++] = msg;
        save_conversation(&msg);
    }
    
    // Send message to clients using new format: MSG|timestamp|sender|recipient|content
    char message[BUFFER_SIZE + MAX_USERNAME * 2];
    snprintf(message, sizeof(message), "MSG|%s|%s|%s|%s", 
             msg.timestamp, sender, recipient, content);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            // For private messages, only send to sender and recipient
            if (strcmp(recipient, "all") != 0) {
                if (strcmp(clients[i].username, sender) == 0 || 
                    strcmp(clients[i].username, recipient) == 0) {
                    send(clients[i].sockfd, message, strlen(message), 0);
                }
            } else {
                // For public messages, send to all except sender
                if (strcmp(clients[i].username, sender) != 0) {
                    send(clients[i].sockfd, message, strlen(message), 0);
                }
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void send_conversation_history(int client_sock, const char *target, const char *username) {
    pthread_mutex_lock(&conversations_mutex);
    
    printf("Debug: Sending conversation history for %s to user %s\n", target, username);
    
    // Find conversation
    server_conversation_t *conv = NULL;
    for (int i = 0; i < conversation_count; i++) {
        // Cho chat công khai, chỉ cần kiểm tra participant1
        if (strcmp(target, "all") == 0) {
            if (strcmp(conversations[i].participant1, "all") == 0) {
                conv = &conversations[i];
                printf("Debug: Found public conversation with %d messages\n", conv->message_count);
                break;
            }
        } else {
            // Cho chat riêng, kiểm tra cả hai participant
            if ((strcmp(conversations[i].participant1, target) == 0) ||
                (strcmp(conversations[i].participant2, target) == 0)) {
                conv = &conversations[i];
                printf("Debug: Found private conversation with %d messages\n", conv->message_count);
                break;
            }
        }
    }
    
    if (conv != NULL) {
        // Gửi header với dấu xuống dòng
        char header[BUFFER_SIZE];
        snprintf(header, sizeof(header), "/history:%s\n", target);
        printf("Debug: Sending header: %s", header);
        if (send(client_sock, header, strlen(header), 0) < 0) {
            printf("Error: Failed to send history header\n");
            pthread_mutex_unlock(&conversations_mutex);
            return;
        }
        
        // Tính toán start_index để chỉ lấy 5 tin nhắn gần nhất
        int start_index = (conv->message_count > 5) ? (conv->message_count - 5) : 0;
        printf("Debug: Sending messages from index %d to %d\n", start_index, conv->message_count - 1);
        
        // Gửi từng tin nhắn, bắt đầu từ start_index
        for (int i = start_index; i < conv->message_count; i++) {
            char message[BUFFER_SIZE * 2];
            if (conv->messages[i].is_private) {
                // Tin nhắn riêng tư: chỉ gửi nếu người dùng là người gửi hoặc người nhận
                if (strcmp(username, conv->messages[i].sender) == 0 ||
                    strcmp(username, conv->messages[i].recipient) == 0) {
                    snprintf(message, sizeof(message), "MSG|%s|%s|%s|%s\n",
                            conv->messages[i].timestamp,
                            conv->messages[i].sender,
                            conv->messages[i].recipient,
                            conv->messages[i].content);
                    printf("Debug: Sending private message: %s", message);
                    if (send(client_sock, message, strlen(message), 0) < 0) {
                        printf("Error: Failed to send private message\n");
                    }
                }
            } else {
                // Tin nhắn công khai: gửi tất cả
                snprintf(message, sizeof(message), "MSG|%s|%s|%s|%s\n",
                        conv->messages[i].timestamp,
                        conv->messages[i].sender,
                        "all",
                        conv->messages[i].content);
                printf("Debug: Sending public message: %s", message);
                if (send(client_sock, message, strlen(message), 0) < 0) {
                    printf("Error: Failed to send public message\n");
                }
            }
        }
        printf("Debug: Finished sending conversation history\n");
    } else {
        printf("Debug: No conversation found for target %s\n", target);
    }
    
    pthread_mutex_unlock(&conversations_mutex);
} 