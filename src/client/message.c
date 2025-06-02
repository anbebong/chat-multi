#include "client.h"

void print_message(const char *sender, const char *content, int is_system, const char *recipient) {
    pthread_mutex_lock(&display_mutex);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", t);
    
    // If this is our own message, just show a checkmark
    if (strcmp(sender, my_username) == 0 && !is_system) {
        printf("\r[%s] ✓\n", timestamp);
    } else {
        // For system messages
        if (is_system) {
            printf("\r[%s] %s\n", timestamp, content);
        }
        // For public messages
        else if (strcmp(recipient, "all") == 0) {
            printf("\r[%s] %s: %s\n", timestamp, sender, content);
        }
        // For private messages
        else {
            printf("\r[%s] %s (to %s): %s\n", timestamp, sender, recipient, content);
        }
    }
    
    print_prompt();
    fflush(stdout);
    
    pthread_mutex_unlock(&display_mutex);
}

void print_prompt() {
    if (strcmp(current_recipient, "all") == 0) {
        printf("[%s] > ", my_username);
    } else {
        printf("[%s -> %s] > ", my_username, current_recipient);
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
    int n;
    
    while ((n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        
        // Handle user list
        if (strncmp(buffer, "USERS:", 6) == 0) {
            char *user_list = buffer + 6;
            update_user_list(user_list);
            continue;
        }
        
        // Handle conversation history
        if (strncmp(buffer, "HISTORY:", 8) == 0) {
            char *history = buffer + 8;
            char *target = strtok(history, ":");
            if (target != NULL) {
                char *messages = strtok(NULL, "");
                if (messages != NULL) {
                    load_conversation_history(target, messages);
                }
            }
            continue;
        }
        
        // Handle normal messages
        char *sender = strtok(buffer, ":");
        if (sender != NULL) {
            char *recipient = strtok(NULL, ":");
            if (recipient != NULL) {
                char *content = strtok(NULL, "");
                if (content != NULL) {
                    // Skip our own messages (they're already displayed)
                    if (strcmp(sender, my_username) != 0) {
                        print_message(sender, content, 0, recipient);
                        
                        // Add to conversation history
                        add_to_conversation(sender, content, recipient,
                                         strcmp(recipient, "all") != 0);
                        
                        // Update unread count if not in this conversation
                        if (strcmp(recipient, current_recipient) != 0) {
                            update_unread_count(sender, 1);
                        }
                    }
                }
            }
        }
    }
    
    if (n <= 0) {
        print_message("System", "Disconnected from server", 1, "all");
        exit(1);
    }
    
    return NULL;
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