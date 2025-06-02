#include "client.h"

void add_to_conversation(const char *sender, const char *content, const char *recipient, int is_private) {
    pthread_mutex_lock(&display_mutex);
    
    time_t now = time(NULL);
    int conv_index = -1;
    
    // Find existing conversation
    for (int i = 0; i < conversation_count; i++) {
        if (strcmp(conversations[i].username, recipient) == 0) {
            conv_index = i;
            break;
        }
    }
    
    // Create new conversation if not found
    if (conv_index == -1 && conversation_count < MAX_CONVERSATIONS) {
        conv_index = conversation_count++;
        strcpy(conversations[conv_index].username, recipient);
        conversations[conv_index].message_count = 0;
        conversations[conv_index].unread_count = 0;
        conversations[conv_index].last_message_time = now;
    }
    
    // Add message if conversation exists
    if (conv_index != -1 && conversations[conv_index].message_count < MAX_MESSAGES) {
        conversation_t *conv = &conversations[conv_index];
        message_t *msg = &conv->messages[conv->message_count++];
        
        strcpy(msg->username, sender);
        strcpy(msg->content, content);
        strcpy(msg->recipient, recipient);
        msg->timestamp = now;
        msg->is_private = is_private;
        
        conv->last_message_time = now;
        
        // Update unread count if message is not from us
        if (strcmp(sender, my_username) != 0) {
            conv->unread_count++;
        }
    }
    
    pthread_mutex_unlock(&display_mutex);
}

void print_active_conversations() {
    pthread_mutex_lock(&display_mutex);
    
    printf("\n=== Active Conversations ===\n");
    
    if (conversation_count == 0) {
        printf("No active conversations\n");
    } else {
        for (int i = 0; i < conversation_count; i++) {
            conversation_t *conv = &conversations[i];
            char time_str[20];
            struct tm *t = localtime(&conv->last_message_time);
            strftime(time_str, sizeof(time_str), "%H:%M:%S", t);
            
            printf("%s (%d unread) - Last: %s\n", 
                   conv->username, conv->unread_count, time_str);
        }
    }
    
    printf("===========================\n\n");
    pthread_mutex_unlock(&display_mutex);
}

void load_conversation_history(const char *target, const char *history) {
    pthread_mutex_lock(&display_mutex);
    
    // Find or create conversation
    int conv_index = -1;
    for (int i = 0; i < conversation_count; i++) {
        if (strcmp(conversations[i].username, target) == 0) {
            conv_index = i;
            break;
        }
    }
    
    if (conv_index == -1 && conversation_count < MAX_CONVERSATIONS) {
        conv_index = conversation_count++;
        strcpy(conversations[conv_index].username, target);
        conversations[conv_index].message_count = 0;
        conversations[conv_index].unread_count = 0;
        conversations[conv_index].last_message_time = time(NULL);
    }
    
    if (conv_index != -1) {
        conversation_t *conv = &conversations[conv_index];
        
        // Parse history string
        char *history_copy = strdup(history);
        char *message = strtok(history_copy, "\n");
        
        while (message != NULL && conv->message_count < MAX_MESSAGES) {
            char sender[MAX_USERNAME];
            char content[BUFFER_SIZE];
            time_t timestamp;
            
            // Parse message format: "timestamp:sender:content"
            char *timestamp_str = strtok(message, ":");
            char *sender_str = strtok(NULL, ":");
            char *content_str = strtok(NULL, "");
            
            if (timestamp_str && sender_str && content_str) {
                timestamp = atol(timestamp_str);
                
                message_t *msg = &conv->messages[conv->message_count++];
                strcpy(msg->username, sender_str);
                strcpy(msg->content, content_str);
                strcpy(msg->recipient, target);
                msg->timestamp = timestamp;
                msg->is_private = 1;  // History messages are always private
            }
            
            message = strtok(NULL, "\n");
        }
        
        free(history_copy);
    }
    
    pthread_mutex_unlock(&display_mutex);
}

void update_user_list(const char *user_list) {
    pthread_mutex_lock(&display_mutex);
    
    // Clear existing users
    user_count = 0;
    
    // Parse user list
    char *list_copy = strdup(user_list);
    char *username = strtok(list_copy, ",");
    while (username != NULL && user_count < MAX_USERS) {
        strcpy(users[user_count].username, username);
        users[user_count].unread_count = 0;
        user_count++;
        username = strtok(NULL, ",");
    }
    free(list_copy);
    
    // Print user list
    printf("\nOnline Users:\n");
    printf("-------------\n");
    for (int i = 0; i < user_count; i++) {
        printf("%s%s\n", users[i].username, 
               strcmp(users[i].username, my_username) == 0 ? " (you)" : "");
    }
    printf("-------------\n");
    print_prompt();
    
    pthread_mutex_unlock(&display_mutex);
}

void cleanup() {
    // Nothing to clean up for now since we're not using local storage
    printf("\nGoodbye!\n");
} 