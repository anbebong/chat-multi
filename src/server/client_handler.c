#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "server.h"

extern client_t clients[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t clients_mutex;
extern server_conversation_t conversations[MAX_CONVERSATIONS];
extern int conversation_count;
extern pthread_mutex_t conversations_mutex;

// Client handling functions
void add_client(int sockfd, const char *username, const char *password) {
    pthread_mutex_lock(&clients_mutex);

    if (is_user_online(username)) {
        char *error_msg = "Error: User is already connected\n";
        send(sockfd, error_msg, strlen(error_msg), 0);
        close(sockfd);
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    if (user_exists(username)) {
        // User cũ: kiểm tra password
        printf("[DEBUG] User %s exists, checking password...\n", username);
        if (!check_user_password(username, password)) {
            char *error_msg = "Error: Invalid password\n";
            send(sockfd, error_msg, strlen(error_msg), 0);
            close(sockfd);
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    } else {
        // User mới: đăng ký
        printf("[DEBUG] User %s does not exist, registering new user...\n", username);
        register_user(username, password);
    }

    // Thêm vào mảng clients
    int index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        clients[index].sockfd = sockfd;
        strcpy(clients[index].username, username);
        strcpy(clients[index].password, password);
        clients[index].active = 1;
        pthread_mutex_init(&clients[index].mutex, NULL);
        if (index >= client_count) client_count = index + 1;
        printf("Debug: Added client %s\n", username);
    } else {
        char *error_msg = "Error: Server is full\n";
        send(sockfd, error_msg, strlen(error_msg), 0);
        close(sockfd);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd == client_socket) {
            printf("Client %s disconnected\n", clients[i].username);
            char message[BUFFER_SIZE];
            snprintf(message, BUFFER_SIZE, "System:all:%s has left the chat\n", clients[i].username);
            broadcast_message_to_socket(message, client_socket, "all");
            
            close(clients[i].sockfd);
            clients[i].active = 0;
            
            // Update user list for all clients
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].active && clients[j].sockfd != client_socket) {
                    send_user_list(clients[j].sockfd);
                }
            }
            
            // Save updated user list to file
            save_users();
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

int find_client(const char *username) {
    pthread_mutex_lock(&clients_mutex);
    
    int index = -1;
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            index = i;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    return index;
}

void notify_user_joined(const char *username) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "System:%s has joined the chat", username);
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) != 0) {
            if (send(clients[i].sockfd, message, strlen(message), 0) < 0) {
                printf("Error sending join notification to %s\n", clients[i].username);
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void notify_user_left(const char *username) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "System:%s has left the chat", username);
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) != 0) {
            if (send(clients[i].sockfd, message, strlen(message), 0) < 0) {
                printf("Error sending leave notification to %s\n", clients[i].username);
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void send_user_list(int client_sock) {
    // Create a temporary buffer for the user list
    char user_list[BUFFER_SIZE] = "/users:";
    int pos = 7;  // Length of "/users:"
    int first = 1;
    int active_count = 0;
    
    // Get list of active users with minimal lock time
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            active_count++;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    if (active_count > 0) {
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                if (!first) {
                    user_list[pos++] = ',';
                }
                strcpy(user_list + pos, clients[i].username);
                pos += strlen(clients[i].username);
                first = 0;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    
    user_list[pos++] = '\n';
    user_list[pos] = '\0';
    
    // Send without holding the mutex
    if (send(client_sock, user_list, pos, 0) < 0) {
        printf("Error sending user list to socket\n");
    }
}

// Kiểm tra user có tồn tại trong file users.txt không (chỉ kiểm tra tên)
int user_exists(const char *username) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) return 0;
    char u[MAX_USERNAME], p[MAX_PASSWORD];
    while (fscanf(fp, "%31s %63s", u, p) == 2) {
        if (strcmp(u, username) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// Kiểm tra user và password có khớp không
int check_user_password(const char *username, const char *password) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) return 0;
    char u[MAX_USERNAME], p[MAX_PASSWORD];
    while (fscanf(fp, "%31s %63s", u, p) == 2) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// Kiểm tra user đã online chưa
int is_user_online(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

// Đăng ký user mới vào file
void register_user(const char *username, const char *password) {
    FILE *fp = fopen(USERS_FILE, "a");
    if (fp) {
        fprintf(fp, "%s %s\n", username, password);
        fclose(fp);
        printf("[DEBUG] Registered new user: %s\n", username);
    } else {
        printf("[ERROR] Cannot open %s for writing!\n", USERS_FILE);
    }
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char content[BUFFER_SIZE - MAX_USERNAME * 2 - 3];
    char recipient[MAX_USERNAME];
    
    // Get username and password
    memset(username, 0, MAX_USERNAME);
    memset(password, 0, MAX_PASSWORD);
    
    // First receive username
    int bytes_received = recv(client->sockfd, username, MAX_USERNAME - 1, 0);
    if (bytes_received <= 0) {
        remove_client(client->sockfd);
        pthread_exit(NULL);
    }
    username[bytes_received] = '\0';
    username[strcspn(username, "\r\n")] = 0;
    printf("[DEBUG] Username received: '%s'\n", username);
    for (int i = 0; i < strlen(username); i++) printf("%02X ", (unsigned char)username[i]);
    printf("<END username>\n");
    
    // Then receive password
    bytes_received = recv(client->sockfd, password, MAX_PASSWORD - 1, 0);
    if (bytes_received <= 0) {
        remove_client(client->sockfd);
        pthread_exit(NULL);
    }
    password[bytes_received] = '\0';
    password[strcspn(password, "\r\n")] = 0;
    printf("[DEBUG] Password received: '%s'\n", password);
    for (int i = 0; i < strlen(password); i++) printf("%02X ", (unsigned char)password[i]);
    printf("<END password>\n");
    
    // Gọi add_client để xử lý xác thực và đăng ký user
    add_client(client->sockfd, username, password);
    // Nếu add_client đóng socket (bị từ chối), thì thoát thread
    if (find_client(username) == -1) {
        pthread_exit(NULL);
    }
    
    printf("Debug: User %s joined the chat\n", username);
    
    // Send join message
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "System:all:%s has joined the chat\n", username);
    broadcast_message_to_socket(message, client->sockfd, "all");
    
    // Send current user list to the new client
    send_user_list(client->sockfd);
    
    // Update user list for all other clients
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd != client->sockfd) {
            pthread_mutex_unlock(&clients_mutex);
            send_user_list(clients[i].sockfd);
            pthread_mutex_lock(&clients_mutex);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client->sockfd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            remove_client(client->sockfd);
            break;
        }
        
        // Remove trailing newline if present
        if (buffer[bytes_received - 1] == '\n') {
            buffer[bytes_received - 1] = '\0';
        }
        
        printf("Debug: Received from %s: %s\n", username, buffer);
        
        // Handle special commands
        if (strcmp(buffer, "/users") == 0) {
            send_user_list(client->sockfd);
            continue;
        }
        
        // Parse message format: "USERNAME:RECIPIENT:CONTENT"
        char *first_colon = strchr(buffer, ':');
        if (first_colon != NULL) {
            *first_colon = '\0';
            strncpy(username, buffer, MAX_USERNAME - 1);
            username[MAX_USERNAME - 1] = '\0';
            
            char *second_colon = strchr(first_colon + 1, ':');
            if (second_colon != NULL) {
                *second_colon = '\0';
                strncpy(recipient, first_colon + 1, MAX_USERNAME - 1);
                recipient[MAX_USERNAME - 1] = '\0';
                
                // Calculate remaining space for content
                size_t max_content_len = BUFFER_SIZE - strlen(username) - strlen(recipient) - 3;
                strncpy(content, second_colon + 1, max_content_len - 1);
                content[max_content_len - 1] = '\0';
                
                printf("Debug: Parsed message - From: %s, To: %s, Content: %s\n", 
                       username, recipient, content);
                
                // Format message with username and recipient
                snprintf(message, BUFFER_SIZE, "%s:%s:%s\n", username, recipient, content);
                
                // Send to recipient(s) only, not back to sender
                broadcast_message(username, content, recipient);
            }
        }
        
        // Handle history command
        if (strncmp(buffer, "/history ", 9) == 0) {
            char *target = buffer + 9;
            if (strlen(target) > 0) {
                send_conversation_history(client->sockfd, target, username);
            }
            continue;
        }
    }
    
    pthread_exit(NULL);
} 