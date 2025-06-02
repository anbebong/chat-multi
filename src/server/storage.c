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

void save_conversation(const server_message_t *msg) {
    pthread_mutex_lock(&conversations_mutex);
    
    // Find or create conversation
    int conv_index = -1;
    for (int i = 0; i < conversation_count; i++) {
        if (msg->is_private) {
            // For private messages, match either sender or recipient
            if ((strcmp(conversations[i].participant1, msg->sender) == 0 && 
                 strcmp(conversations[i].participant2, msg->recipient) == 0) ||
                (strcmp(conversations[i].participant1, msg->recipient) == 0 && 
                 strcmp(conversations[i].participant2, msg->sender) == 0)) {
                conv_index = i;
                break;
            }
        } else {
            // For public messages, match "all" as participant
            if (strcmp(conversations[i].participant1, "all") == 0) {
                conv_index = i;
                break;
            }
        }
    }
    
    // Create new conversation if not found
    if (conv_index == -1 && conversation_count < MAX_CONVERSATIONS) {
        conv_index = conversation_count++;
        strcpy(conversations[conv_index].participant1, msg->is_private ? msg->sender : "all");
        strcpy(conversations[conv_index].participant2, msg->is_private ? msg->recipient : "");
        conversations[conv_index].message_count = 0;
        conversations[conv_index].last_active = time(NULL);
    }
    
    // Add message to conversation
    if (conv_index != -1 && conversations[conv_index].message_count < MAX_MESSAGES_PER_CONVERSATION) {
        server_conversation_t *conv = &conversations[conv_index];
        server_message_t *new_msg = &conv->messages[conv->message_count++];
        
        strcpy(new_msg->sender, msg->sender);
        strcpy(new_msg->recipient, msg->recipient);
        strcpy(new_msg->content, msg->content);
        strcpy(new_msg->timestamp, msg->timestamp);
        new_msg->is_private = msg->is_private;
        
        conv->last_active = time(NULL);
    }
    
    pthread_mutex_unlock(&conversations_mutex);
    
    // Save to file
    FILE *fp = fopen(CONVERSATIONS_FILE, "a");
    if (fp != NULL) {
        fprintf(fp, "%s:%s:%s:%s:%d\n",
                msg->sender, msg->recipient, msg->content, 
                msg->timestamp, msg->is_private);
        fclose(fp);
    } else {
        printf("Error: Could not open conversations file for writing\n");
    }
}

void load_conversations() {
    FILE *fp = fopen(CONVERSATIONS_FILE, "r");
    if (fp == NULL) {
        return;
    }
    
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse line
        char *sender = strtok(line, ":");
        if (sender != NULL) {
            char *recipient = strtok(NULL, ":");
            if (recipient != NULL) {
                char *content = strtok(NULL, ":");
                if (content != NULL) {
                    char *timestamp = strtok(NULL, ":");
                    if (timestamp != NULL) {
                        char *is_private_str = strtok(NULL, ":");
                        if (is_private_str != NULL) {
                            server_message_t msg;
                            strcpy(msg.sender, sender);
                            strcpy(msg.recipient, recipient);
                            strcpy(msg.content, content);
                            strcpy(msg.timestamp, timestamp);
                            msg.is_private = atoi(is_private_str);
                            
                            // Add to conversation
                            pthread_mutex_lock(&conversations_mutex);
                            
                            // Find or create conversation
                            int conv_index = -1;
                            for (int i = 0; i < conversation_count; i++) {
                                if (msg.is_private) {
                                    if ((strcmp(conversations[i].participant1, msg.sender) == 0 && 
                                         strcmp(conversations[i].participant2, msg.recipient) == 0) ||
                                        (strcmp(conversations[i].participant1, msg.recipient) == 0 && 
                                         strcmp(conversations[i].participant2, msg.sender) == 0)) {
                                        conv_index = i;
                                        break;
                                    }
                                } else {
                                    if (strcmp(conversations[i].participant1, "all") == 0) {
                                        conv_index = i;
                                        break;
                                    }
                                }
                            }
                            
                            if (conv_index == -1 && conversation_count < MAX_CONVERSATIONS) {
                                conv_index = conversation_count++;
                                strcpy(conversations[conv_index].participant1, 
                                      msg.is_private ? msg.sender : "all");
                                strcpy(conversations[conv_index].participant2, 
                                      msg.is_private ? msg.recipient : "");
                                conversations[conv_index].message_count = 0;
                                conversations[conv_index].last_active = time(NULL);
                            }
                            
                            if (conv_index != -1 && 
                                conversations[conv_index].message_count < MAX_MESSAGES_PER_CONVERSATION) {
                                server_conversation_t *conv = &conversations[conv_index];
                                server_message_t *new_msg = &conv->messages[conv->message_count++];
                                
                                strcpy(new_msg->sender, msg.sender);
                                strcpy(new_msg->recipient, msg.recipient);
                                strcpy(new_msg->content, msg.content);
                                strcpy(new_msg->timestamp, msg.timestamp);
                                new_msg->is_private = msg.is_private;
                                
                                conv->last_active = time(NULL);
                            }
                            
                            pthread_mutex_unlock(&conversations_mutex);
                        }
                    }
                }
            }
        }
    }
    
    fclose(fp);
}

void save_conversations() {
    FILE *fp = fopen(CONVERSATIONS_FILE, "w");
    if (fp == NULL) {
        printf("Error: Could not open conversations file for writing\n");
        return;
    }
    
    pthread_mutex_lock(&conversations_mutex);
    
    for (int i = 0; i < conversation_count; i++) {
        server_conversation_t *conv = &conversations[i];
        for (int j = 0; j < conv->message_count; j++) {
            server_message_t *msg = &conv->messages[j];
            fprintf(fp, "%s:%s:%s:%s:%d\n",
                    msg->sender, msg->recipient, msg->content,
                    msg->timestamp, msg->is_private);
        }
    }
    
    pthread_mutex_unlock(&conversations_mutex);
    
    fclose(fp);
}

void save_users() {
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) {
        printf("Error: Could not open users file for writing\n");
        return;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++) {
        if (strlen(clients[i].username) > 0) {  // Save all users, not just active ones
            fprintf(fp, "%s:%s\n", clients[i].username, clients[i].password);
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    fclose(fp);
}

void load_users() {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        return;
    }
    
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse line (format: username:password)
        char *username = strtok(line, ":");
        if (username != NULL) {
            char *password = strtok(NULL, ":");
            if (password != NULL) {
                // Find free slot
                int index = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (strlen(clients[i].username) == 0) {
                        index = i;
                        break;
                    }
                }
                
                if (index != -1) {
                    strcpy(clients[index].username, username);
                    strcpy(clients[index].password, password);
                    clients[index].active = 0;  // Not active until they connect
                    if (index >= client_count) {
                        client_count = index + 1;
                    }
                }
            }
        }
    }
    
    fclose(fp);
}

void save_server_conversation(const char *username, const server_message_t *messages, int message_count) {
    pthread_mutex_lock(&conversations_mutex);
    
    FILE *file = fopen(CONVERSATIONS_FILE, "a");
    if (file == NULL) {
        printf("Error: Could not open conversations file for writing\n");
        pthread_mutex_unlock(&conversations_mutex);
        return;
    }
    
    // Write conversation header
    fprintf(file, "=== Conversation with %s ===\n", username);
    fprintf(file, "Timestamp: %ld\n", time(NULL));
    
    // Write messages
    for (int i = 0; i < message_count; i++) {
        if (messages[i].is_private) {
            fprintf(file, "[%s] %s (private to %s): %s\n",
                    messages[i].timestamp, messages[i].sender, 
                    messages[i].recipient, messages[i].content);
        } else {
            fprintf(file, "[%s] %s: %s\n",
                    messages[i].timestamp, messages[i].sender, messages[i].content);
        }
    }
    
    fprintf(file, "===========================\n\n");
    fclose(file);
    
    pthread_mutex_unlock(&conversations_mutex);
}

void load_server_conversation_history() {
    pthread_mutex_lock(&conversations_mutex);
    
    FILE *file = fopen(CONVERSATIONS_FILE, "r");
    if (file == NULL) {
        printf("Error: Could not open conversations file for reading\n");
        pthread_mutex_unlock(&conversations_mutex);
        return;
    }
    
    char line[BUFFER_SIZE];
    char current_username[MAX_USERNAME] = "";
    int in_conversation = 0;
    server_conversation_t *current_conv = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Check for conversation header
        if (strncmp(line, "=== Conversation with ", 21) == 0) {
            if (in_conversation && strlen(current_username) > 0) {
                // Save previous conversation
                save_server_conversation(current_username, 
                                      current_conv->messages,
                                      current_conv->message_count);
            }
            
            // Start new conversation
            strncpy(current_username, line + 21, MAX_USERNAME - 1);
            current_username[strcspn(current_username, " ")] = 0;
            
            // Find or create conversation
            current_conv = NULL;
            for (int i = 0; i < conversation_count; i++) {
                if (strcmp(conversations[i].participant1, current_username) == 0 ||
                    strcmp(conversations[i].participant2, current_username) == 0) {
                    current_conv = &conversations[i];
                    break;
                }
            }
            
            if (current_conv == NULL && conversation_count < MAX_CONVERSATIONS) {
                current_conv = &conversations[conversation_count++];
                strncpy(current_conv->participant1, current_username, MAX_USERNAME - 1);
                current_conv->message_count = 0;
                current_conv->last_active = time(NULL);
            }
            
            in_conversation = 1;
        }
        // Parse message line
        else if (in_conversation && current_conv != NULL && 
                 line[0] == '[' && strchr(line, ']') != NULL) {
            char *time_end = strchr(line, ']');
            char *sender_start = time_end + 2;
            char *content_start = strchr(sender_start, ':');
            
            if (content_start != NULL) {
                *content_start = '\0';
                content_start += 2;
                
                server_message_t *msg = &current_conv->messages[current_conv->message_count++];
                strncpy(msg->sender, sender_start, MAX_USERNAME - 1);
                strncpy(msg->content, content_start, BUFFER_SIZE - 1);
                strncpy(msg->timestamp, line + 1, time_end - line - 1);
                
                // Check if it's a private message
                char *private_to = strstr(sender_start, "(private to");
                if (private_to != NULL) {
                    msg->is_private = 1;
                    char *recipient_start = private_to + 12;
                    char *recipient_end = strchr(recipient_start, ')');
                    if (recipient_end != NULL) {
                        *recipient_end = '\0';
                        strncpy(msg->recipient, recipient_start, MAX_USERNAME - 1);
                    }
                } else {
                    msg->is_private = 0;
                    strcpy(msg->recipient, "all");
                }
            }
        }
    }
    
    // Save last conversation if any
    if (in_conversation && strlen(current_username) > 0 && current_conv != NULL) {
        save_server_conversation(current_username, 
                              current_conv->messages,
                              current_conv->message_count);
    }
    
    fclose(file);
    pthread_mutex_unlock(&conversations_mutex);
} 