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
    
    // Save to conversation file
    char filename[BUFFER_SIZE];
    get_conversation_filename(msg->sender, msg->recipient, filename);
    
    FILE *fp = fopen(filename, "a");
    if (fp != NULL) {
        fprintf(fp, "%s:%s:%s:%s:%d\n",
                msg->sender, msg->recipient, msg->content, 
                msg->timestamp, msg->is_private);
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
    // Tạo thư mục conversations nếu chưa tồn tại
    mkdir("conversations", 0755);
    
    // Chuyển dữ liệu từ file cũ sang các file mới
    migrate_old_conversations();
    
    // Đọc tất cả file trong thư mục conversations
    DIR *dir = opendir("conversations");
    if (dir == NULL) {
        printf("Error: Could not open conversations directory\n");
        return;
    }
    
    printf("Debug: Starting to load conversations...\n");
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Bỏ qua . và ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Chỉ đọc file .txt
        if (strstr(entry->d_name, ".txt") == NULL) {
            continue;
        }
        
        char filename[BUFFER_SIZE];
        snprintf(filename, sizeof(filename), "conversations/%s", entry->d_name);
        printf("Debug: Loading conversation from %s\n", filename);
        
        FILE *fp = fopen(filename, "r");
        if (fp == NULL) {
            printf("Error: Could not open conversation file %s\n", filename);
            continue;
        }
        
        // Parse tên file để lấy thông tin người tham gia
        char *participant1;
        char *participant2 = NULL;
        
        if (strcmp(entry->d_name, "all.txt") == 0) {
            participant1 = "all";
        } else {
            participant1 = strtok(entry->d_name, "_");
            participant2 = strtok(NULL, ".");
            if (participant2 != NULL) {
                // Loại bỏ phần .txt
                char *dot = strchr(participant2, '.');
                if (dot != NULL) {
                    *dot = '\0';
                }
            }
        }
        
        printf("Debug: Conversation participants: %s and %s\n", 
               participant1, participant2 ? participant2 : "all");
        
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
                
                // Parse line
                char *sender = strtok(line, ":");
                if (sender != NULL) {
                    char *recipient = strtok(NULL, ":");
                    if (recipient != NULL) {
                        char *content = strtok(NULL, ":");
                        if (content != NULL) {
                            // Tìm vị trí của timestamp (sau content)
                            char *timestamp_start = content + strlen(content) + 1;
                            if (timestamp_start < line + strlen(line)) {
                                // Tìm vị trí của is_private (sau timestamp)
                                char *is_private_start = strrchr(timestamp_start, ':');
                                int is_private = 0;
                                
                                if (is_private_start != NULL) {
                                    // Có trường is_private
                                    *is_private_start = '\0';  // Cắt timestamp
                                    is_private = atoi(is_private_start + 1);
                                    is_private = (is_private != 0) ? 1 : 0;
                                } else {
                                    // Không có trường is_private
                                    is_private = (strcmp(recipient, "all") != 0);
                                }
                                
                                server_message_t *msg = &conv->messages[conv->message_count++];
                                strcpy(msg->sender, sender);
                                strcpy(msg->recipient, recipient);
                                strcpy(msg->content, content);
                                strcpy(msg->timestamp, timestamp_start);
                                msg->is_private = is_private;
                                
                                msg_count++;
                                printf("Debug: Loaded message %d - From: %s, To: %s, Content: %s, Time: %s, Private: %d\n",
                                       msg_count, msg->sender, msg->recipient, msg->content, 
                                       msg->timestamp, msg->is_private);
                            }
                        }
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