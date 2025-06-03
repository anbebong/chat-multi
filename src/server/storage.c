#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include "server.h"

extern client_t clients[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t clients_mutex;
extern server_conversation_t conversations[MAX_CONVERSATIONS];
extern int conversation_count;
extern pthread_mutex_t conversations_mutex;

// Tạo tên file cho cuộc hội thoại
void get_conversation_filename(const char *user1, const char *user2, char *filename) {
    // Tạo thư mục conversations nếu chưa tồn tại
    mkdir("conversations", 0755);
    
    if (strcmp(user1, "all") == 0 || strcmp(user2, "all") == 0) {
        // Chat công khai
        snprintf(filename, BUFFER_SIZE, "conversations/all.txt");
    } else {
        // Chat riêng: sắp xếp tên user theo thứ tự alphabet để tránh trùng lặp
        if (strcmp(user1, user2) < 0) {
            snprintf(filename, BUFFER_SIZE, "conversations/%s_%s.txt", user1, user2);
        } else {
            snprintf(filename, BUFFER_SIZE, "conversations/%s_%s.txt", user2, user1);
        }
    }
}

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
    
    // Save to conversation file using new format: MSG|timestamp|sender|recipient|content|is_private
    char filename[BUFFER_SIZE];
    get_conversation_filename(msg->sender, msg->recipient, filename);
    
    FILE *fp = fopen(filename, "a");
    if (fp != NULL) {
        fprintf(fp, "MSG|%s|%s|%s|%s|%d\n",
                msg->timestamp, msg->sender, msg->recipient, 
                msg->content, msg->is_private);
        fclose(fp);
    } else {
        printf("Error: Could not open conversation file %s for writing\n", filename);
    }
}

// Chuyển dữ liệu từ file cũ sang các file mới
void migrate_old_conversations() {
    FILE *fp = fopen(CONVERSATIONS_FILE, "r");
    if (fp == NULL) {
        return;  // Không có file cũ
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
                            // Tạo message từ dữ liệu cũ
                            server_message_t msg;
                            strcpy(msg.sender, sender);
                            strcpy(msg.recipient, recipient);
                            strcpy(msg.content, content);
                            strcpy(msg.timestamp, timestamp);
                            msg.is_private = atoi(is_private_str);
                            
                            // Lưu vào file mới
                            char filename[BUFFER_SIZE];
                            get_conversation_filename(sender, recipient, filename);
                            
                            FILE *new_fp = fopen(filename, "a");
                            if (new_fp != NULL) {
                                fprintf(new_fp, "%s:%s:%s:%s:%d\n",
                                        msg.sender, msg.recipient, msg.content,
                                        msg.timestamp, msg.is_private);
                                fclose(new_fp);
                            }
                        }
                    }
                }
            }
        }
    }
    
    fclose(fp);
    
    // Xóa file cũ sau khi đã chuyển xong
    remove(CONVERSATIONS_FILE);
}

void load_conversations() {
    DIR *dir = opendir("conversations");
    if (dir == NULL) {
        printf("Error: Could not open conversations directory\n");
        return;
    }
    
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip . and .. directories
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        
        // Skip non-txt files
        if (strstr(ent->d_name, ".txt") == NULL) {
            continue;
        }
        
        char filename[BUFFER_SIZE];
        snprintf(filename, sizeof(filename), "conversations/%s", ent->d_name);
        
        FILE *fp = fopen(filename, "r");
        if (fp == NULL) {
            printf("Error: Could not open conversation file %s\n", filename);
            continue;
        }
        
        // Parse participants from filename
        char participant1[MAX_USERNAME] = "";
        char participant2[MAX_USERNAME] = "";
        
        if (strcmp(ent->d_name, "all.txt") == 0) {
            strcpy(participant1, "all");
            participant2[0] = '\0';
        } else {
            char *p1 = strtok(ent->d_name, "_");
            char *p2 = strtok(NULL, ".");
            if (p1 && p2) {
                strncpy(participant1, p1, MAX_USERNAME - 1);
                strncpy(participant2, p2, MAX_USERNAME - 1);
            }
        }
        
        printf("Debug: Loading conversation between %s and %s\n", 
               participant1, participant2 ? participant2 : "none");
        
        // Tạo conversation mới
        if (conversation_count < MAX_CONVERSATIONS) {
            server_conversation_t *conv = &conversations[conversation_count++];
            strcpy(conv->participant1, participant1);
            strcpy(conv->participant2, participant2 ? participant2 : "");
            conv->message_count = 0;
            conv->last_active = time(NULL);
            
            // Đọc tin nhắn
            char line[BUFFER_SIZE];
            int msg_count = 0;
            while (fgets(line, sizeof(line), fp) != NULL && 
                   conv->message_count < MAX_MESSAGES_PER_CONVERSATION) {
                // Remove newline
                line[strcspn(line, "\n")] = 0;
                
                // Parse line using new format: MSG|timestamp|sender|recipient|content|is_private
                if (strncmp(line, "MSG|", 4) == 0) {
                    char *msg_content = line + 4;  // Skip "MSG|"
                    
                    // Parse message components using strtok with | as delimiter
                    char *timestamp = strtok(msg_content, "|");
                    char *sender = strtok(NULL, "|");
                    char *recipient = strtok(NULL, "|");
                    char *content = strtok(NULL, "|");
                    char *is_private_str = strtok(NULL, "|");
                    
                    if (timestamp && sender && recipient && content) {
                        int is_private = 0;
                        if (is_private_str) {
                            is_private = atoi(is_private_str);
                            is_private = (is_private != 0) ? 1 : 0;
                        } else {
                            is_private = (strcmp(recipient, "all") != 0);
                        }
                        
                        server_message_t *msg = &conv->messages[conv->message_count++];
                        strcpy(msg->sender, sender);
                        strcpy(msg->recipient, recipient);
                        strcpy(msg->content, content);
                        strcpy(msg->timestamp, timestamp);
                        msg->is_private = is_private;
                        
                        msg_count++;
                        printf("Debug: Loaded message %d - From: %s, To: %s, Content: %s, Time: %s, Private: %d\n",
                               msg_count, msg->sender, msg->recipient, msg->content, 
                               msg->timestamp, msg->is_private);
                    }
                }
            }
            printf("Debug: Loaded %d messages from %s\n", msg_count, filename);
        }
        
        fclose(fp);
    }
    
    closedir(dir);
    printf("Debug: Finished loading %d conversations\n", conversation_count);
}

void save_users() {
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) {
        printf("Error: Could not open users file for writing\n");
        return;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    // Save all registered users (those who have successfully logged in at least once)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strlen(clients[i].username) > 0 && strlen(clients[i].password) > 0) {
            fprintf(fp, "%s %s\n", clients[i].username, clients[i].password);
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