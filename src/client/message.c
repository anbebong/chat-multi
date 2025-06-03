#include "client.h"

void print_message(const char *sender, const char *content, int is_system, const char *recipient) {
    pthread_mutex_lock(&display_mutex);
    
    // If this is our own message, just show a checkmark
    if (strcmp(sender, my_username) == 0 && !is_system) {
        printf("\r\033[K✓\n");
    } else {
        // For system messages
        if (is_system) {
            printf("\r\033[KSystem: %s\n", content);
        }
        // For all messages
        else {
            printf("\r\033[K[%s] > [%s] \"%s\"\n", sender, recipient, content);
        }
    }
    
    // Don't print prompt here, it will be printed by the input thread
    fflush(stdout);
    
    pthread_mutex_unlock(&display_mutex);
}

void print_prompt() {
    if (strcmp(current_recipient, "all") == 0) {
        printf("[%s] > [all] ", my_username);
    } else {
        printf("[%s] > [%s] ", my_username, current_recipient);
    }
    fflush(stdout);
}

void print_help() {
    printf("\nAvailable commands:\n");
    printf("/help     - Show this help message\n");
    printf("/quit     - Exit the chat\n");
    printf("/clear    - Clear the screen\n");
    printf("/users    - Show online users\n");
    printf("/conv     - Show active conversations\n");
    printf("/to user  - Start private chat with user\n");
    printf("/all      - Switch to public chat\n");
    printf("/history user - Show chat history with user\n\n");
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        
        // Handle user list
        if (strncmp(buffer, "/users:", 7) == 0) {
            char *user_list = buffer + 7;  // Skip "/users:"
            pthread_mutex_lock(&display_mutex);
            printf("\n=== Online Users ===\n");
            char *user = strtok(user_list, ",");
            while (user != NULL) {
                printf("- %s\n", user);
                user = strtok(NULL, ",");
            }
            printf("===================\n");
            print_prompt();
            fflush(stdout);
            pthread_mutex_unlock(&display_mutex);
            continue;
        }
        
        // Handle chat history
        if (strncmp(buffer, "/history:", 9) == 0) {
            char *target = buffer + 9;  // Skip "/history:"
            char *messages = strchr(target, '\n');  // Tìm ký tự xuống dòng đầu tiên
            if (messages != NULL) {
                *messages = '\0';  // Split target và messages
                messages++;  // Chuyển đến phần tin nhắn
                
                pthread_mutex_lock(&display_mutex);
                printf("\n=== Chat History with %s ===\n", target);
                
                // Xử lý từng dòng tin nhắn
                char *line = strtok(messages, "\n");
                while (line != NULL) {
                    if (strncmp(line, "/msg:", 5) == 0) {
                        // Bỏ qua header /msg:
                        char *msg_content = line + 5;
                        printf("%s\n", msg_content);
                    }
                    line = strtok(NULL, "\n");
                }
                
                printf("=== End of history ===\n");
                print_prompt();
                fflush(stdout);
                pthread_mutex_unlock(&display_mutex);
            }
            continue;
        }
        
        // Handle normal messages - new format "MSG|timestamp|sender|recipient|content"
        if (strncmp(buffer, "MSG|", 4) == 0) {
            char *msg_content = buffer + 4;  // Skip "MSG|"
            
            // Parse message components using strtok with | as delimiter
            char *timestamp = strtok(msg_content, "|");
            char *sender = strtok(NULL, "|");
            char *recipient = strtok(NULL, "|");
            char *content = strtok(NULL, "\n");  // Get remaining content
            
            if (timestamp && sender && recipient && content) {
                pthread_mutex_lock(&display_mutex);
                printf("\r\033[K[%s] [%s]>[%s]: %s\n", 
                       timestamp, sender, recipient, content);
                fflush(stdout);
                pthread_mutex_unlock(&display_mutex);
            }
            continue;
        }
        
        // Handle system messages
        if (strncmp(buffer, "SYS|", 4) == 0) {
            char *content = buffer + 4;  // Skip "SYS|"
            pthread_mutex_lock(&display_mutex);
            printf("\r\033[KSystem: %s\n", content);
            fflush(stdout);
            pthread_mutex_unlock(&display_mutex);
            continue;
        }
        
        // Unknown message format
        pthread_mutex_lock(&display_mutex);
        printf("\r\033[KReceived unknown message format: %s\n", buffer);
        fflush(stdout);
        pthread_mutex_unlock(&display_mutex);
    }
    
    if (bytes_received == 0) {
        pthread_mutex_lock(&display_mutex);
        printf("\r\033[K\nServer disconnected\n");
        fflush(stdout);
        pthread_mutex_unlock(&display_mutex);
    } else {
        pthread_mutex_lock(&display_mutex);
        perror("\r\033[KError receiving message");
        fflush(stdout);
        pthread_mutex_unlock(&display_mutex);
    }
    
    exit(1);
    return NULL;
}

void send_message(int sockfd, const char *message, const char *recipient) {
    char buffer[BUFFER_SIZE];
    
    // Handle special commands
    if (strncmp(message, "/users", 6) == 0) {
        send(sockfd, "/users", 6, 0);
        return;
    }
    
    if (strncmp(message, "/history", 8) == 0) {
        char *target = strchr(message, ' ');
        if (target != NULL) {
            target++;  // Skip space
            snprintf(buffer, sizeof(buffer), "/history:%s", target);
            send(sockfd, buffer, strlen(buffer), 0);
        }
        return;
    }
    
    // Normal message - send in format "MSG|sender|recipient|content"
    snprintf(buffer, sizeof(buffer), "MSG|%s|%s|%s", my_username, recipient, message);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send message");
    }
}

void update_unread_count(const char *sender, int increment) {
    pthread_mutex_lock(&display_mutex);
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, sender) == 0) {
            users[i].unread_count += increment;
            break;
        }
    }
    
    pthread_mutex_unlock(&display_mutex);
} 
