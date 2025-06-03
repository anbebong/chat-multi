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

// Client handling functions
void add_client(int sockfd, const char *username, const char *password) {
    pthread_mutex_lock(&clients_mutex);

    if (is_user_online(username)) {
        char *error_msg = "Error: User is already connected\n";
        if (send(sockfd, error_msg, strlen(error_msg), 0) < 0) {
            perror("Error sending error message");
        }
        usleep(100000);  // 100ms
        close(sockfd);
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    if (user_exists(username)) {
        // User cũ: kiểm tra password
        printf("[DEBUG] User %s exists, checking password...\n", username);
        if (!check_user_password(username, password)) {
            char *error_msg = "Error: Invalid password\n";
            if (send(sockfd, error_msg, strlen(error_msg), 0) < 0) {
                perror("Error sending error message");
            }
            usleep(100000);  // 100ms
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

    if (index == -1) {
        char *error_msg = "Error: Server is full\n";
        if (send(sockfd, error_msg, strlen(error_msg), 0) < 0) {
            perror("Error sending error message");
        }
        usleep(100000);  // 100ms
        close(sockfd);
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    // Thêm client mới
    clients[index].sockfd = sockfd;
    strcpy(clients[index].username, username);
    strcpy(clients[index].password, password);
    clients[index].active = 1;
    pthread_mutex_init(&clients[index].mutex, NULL);
    if (index >= client_count) client_count = index + 1;
    printf("Debug: Added client %s\n", username);

    pthread_mutex_unlock(&clients_mutex);

    // Gửi thông báo thành công sau khi unlock mutex
    char *success_msg = "Success: Authentication successful\n";
    if (send(sockfd, success_msg, strlen(success_msg), 0) < 0) {
        perror("Error sending success message");
        pthread_mutex_lock(&clients_mutex);
        clients[index].active = 0;
        pthread_mutex_unlock(&clients_mutex);
        close(sockfd);
        return;
    }
}

void remove_client(int client_socket) {
    char disconnected_username[MAX_USERNAME] = "";
    int other_clients[MAX_CLIENTS];
    int other_count = 0;
    
    // First, get the username and prepare the message
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd == client_socket) {
            strncpy(disconnected_username, clients[i].username, MAX_USERNAME - 1);
            disconnected_username[MAX_USERNAME - 1] = '\0';
            
            // Get list of other active clients
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].active && clients[j].sockfd != client_socket) {
                    other_clients[other_count++] = clients[j].sockfd;
                }
            }
            
            // Mark this client as inactive
            close(clients[i].sockfd);
            clients[i].active = 0;  // Chỉ cần đánh dấu là không hoạt động
            
            printf("Client %s disconnected\n", disconnected_username);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // If we found a disconnected client, notify others
    if (strlen(disconnected_username) > 0) {
        // Prepare leave message
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "System:all:%s has left the chat\n", disconnected_username);
        
        // Send leave notification to other clients
        for (int i = 0; i < other_count; i++) {
            if (send(other_clients[i], message, strlen(message), 0) < 0) {
                printf("Error sending leave notification\n");
            }
            // Send updated user list
            send_user_list(other_clients[i]);
        }
        
        // Save updated user list to file
        save_users();
    }
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
    char client_username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    
    // Get username and password
    memset(client_username, 0, MAX_USERNAME);
    memset(password, 0, MAX_PASSWORD);
    
    // Read until newline for username
    int pos = 0;
    while (pos < MAX_USERNAME - 1) {
        int n = recv(client->sockfd, client_username + pos, 1, 0);
        if (n <= 0) {
            remove_client(client->sockfd);
            pthread_exit(NULL);
        }
        if (client_username[pos] == '\n') {
            client_username[pos] = '\0';
            break;
        }
        pos++;
    }
    client_username[strcspn(client_username, "\r")] = 0;
    printf("[DEBUG] Username received: '%s'\n", client_username);
    
    // Read until newline for password
    pos = 0;
    while (pos < MAX_PASSWORD - 1) {
        int n = recv(client->sockfd, password + pos, 1, 0);
        if (n <= 0) {
            remove_client(client->sockfd);
            pthread_exit(NULL);
        }
        if (password[pos] == '\n') {
            password[pos] = '\0';
            break;
        }
        pos++;
    }
    password[strcspn(password, "\r")] = 0;
    printf("[DEBUG] Password received: '%s'\n", password);
    
    // Gọi add_client để xử lý xác thực và đăng ký user
    add_client(client->sockfd, client_username, password);
    
    // Nếu add_client đóng socket (bị từ chối), thì thoát thread
    if (find_client(client_username) == -1) {
        pthread_exit(NULL);
    }
    
    printf("Debug: User %s joined the chat\n", client_username);
    
    // Send join message
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "SYS|%s has joined the chat\n", client_username);
    broadcast_message_to_socket(message, client->sockfd, "all");
    
    // Send current user list to the new client
    send_user_list(client->sockfd);
    
    // Send recent chat history to the new client
    // First send public chat history
    send_conversation_history(client->sockfd, "all", client_username);
    
    // Then send private chat history with each online user
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, client_username) != 0) {
            send_conversation_history(client->sockfd, clients[i].username, client_username);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Update user list for all other clients
    int other_clients[MAX_CLIENTS];
    int other_count = 0;
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd != client->sockfd) {
            other_clients[other_count++] = clients[i].sockfd;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Send user list to other clients without holding the mutex
    for (int i = 0; i < other_count; i++) {
        send_user_list(other_clients[i]);
    }
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client->sockfd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            remove_client(client->sockfd);
            break;
        }
        
        // Remove trailing newline if present
        if (buffer[bytes_received - 1] == '\n') {
            buffer[bytes_received - 1] = '\0';
        }
        
        printf("Debug: Received from %s: %s\n", client_username, buffer);
        
        // Handle special commands
        if (strcmp(buffer, "/users") == 0) {
            send_user_list(client->sockfd);
            continue;
        }
        
        // Handle history command
        if (strncmp(buffer, "/history ", 9) == 0) {
            char *target = buffer + 9;
            if (strlen(target) > 0) {
                send_conversation_history(client->sockfd, target, client_username);
            }
            continue;
        }
        
        // Parse message format: "MSG|sender|recipient|content"
        if (strncmp(buffer, "MSG|", 4) == 0) {
            char *msg_content = buffer + 4;  // Skip "MSG|"
            
            // Parse message components using strtok with | as delimiter
            char *sender = strtok(msg_content, "|");
            char *recipient = strtok(NULL, "|");
            char *content = strtok(NULL, "\n");  // Get remaining content
            
            if (sender && recipient && content) {
                printf("Debug: Đã nhận tin nhắn - Từ: %s, Đến: %s, Nội dung: %s\n", 
                       sender, recipient, content);
                
                // Broadcast message to appropriate clients
                broadcast_message(sender, content, recipient);
            }
            continue;
        }
        
        // Unknown message format
        printf("Debug: Received unknown message format: %s\n", buffer);
    }
    
    pthread_exit(NULL);
} 