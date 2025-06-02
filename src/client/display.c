#include "client.h"

void print_welcome() {
    printf("\n=== Welcome to Chat ===\n");
    printf("Connected as: %s\n", my_username);
    printf("Type /help for available commands\n\n");
}

void print_disconnect() {
    printf("\nDisconnected from server\n");
}

void print_error(const char *msg) {
    printf("\nError: %s\n", msg);
}

void print_system(const char *msg) {
    printf("\nSystem: %s\n", msg);
}

void print_user_joined(const char *username) {
    printf("\n%s has joined the chat\n", username);
}

void print_user_left(const char *username) {
    printf("\n%s has left the chat\n", username);
}

void print_private_start(const char *username) {
    printf("\nStarted private chat with %s\n", username);
}

void print_private_end(const char *username) {
    printf("\nEnded private chat with %s\n", username);
}

void print_no_such_user(const char *username) {
    printf("\nUser %s is not online\n", username);
}

void print_no_history(const char *username) {
    printf("\nNo chat history with %s\n", username);
}

void print_invalid_command() {
    printf("\nInvalid command. Type /help for available commands\n");
}

void print_usage() {
    printf("\nUsage: ./client\n");
}

void print_version() {
    printf("\nChat Client v1.0\n");
}

void print_credits() {
    printf("\nChat Client\n");
    printf("Created by Your Name\n");
    printf("Copyright (c) 2024\n\n");
}

void print_debug(const char *msg) {
#ifdef DEBUG
    printf("\n[DEBUG] %s\n", msg);
#endif
}

void print_debug_hex(const char *prefix, const unsigned char *data, size_t len) {
#ifdef DEBUG
    printf("\n[DEBUG] %s: ", prefix);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
#endif
}

void print_debug_str(const char *prefix, const char *str) {
#ifdef DEBUG
    printf("\n[DEBUG] %s: %s\n", prefix, str);
#endif
}

void print_debug_int(const char *prefix, int value) {
#ifdef DEBUG
    printf("\n[DEBUG] %s: %d\n", prefix, value);
#endif
}

void print_debug_time(const char *prefix, time_t t) {
#ifdef DEBUG
    char time_str[20];
    struct tm *tm = localtime(&t);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    printf("\n[DEBUG] %s: %s\n", prefix, time_str);
#endif
}

void print_debug_user(const char *prefix, const user_t *user) {
#ifdef DEBUG
    printf("\n[DEBUG] %s: username=%s, unread=%d\n", 
           prefix, user->username, user->unread_count);
#endif
}

void print_debug_message(const char *prefix, const message_t *msg) {
#ifdef DEBUG
    printf("\n[DEBUG] %s: username=%s, content=%s, timestamp=%ld\n",
           prefix, msg->username, msg->content, msg->timestamp);
#endif
}

void print_debug_conversation(const char *prefix, const conversation_t *conv) {
#ifdef DEBUG
    printf("\n[DEBUG] %s: username=%s, msg_count=%d, last_message_time=%ld\n",
           prefix, conv->username, conv->message_count, conv->last_message_time);
#endif
}

void print_conversation_history(const char *target) {
    // Find the conversation with the target user
    for (int i = 0; i < conversation_count; i++) {
        conversation_t *conv = &conversations[i];
        if (strcmp(conv->username, target) == 0) {
            printf("\n=== Chat history with %s ===\n", target);
            
            if (conv->message_count == 0) {
                printf("No messages yet\n");
                return;
            }
            
            // Print all messages in the conversation
            for (int j = 0; j < conv->message_count; j++) {
                message_t *msg = &conv->messages[j];
                char time_str[20];
                strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&msg->timestamp));
                printf("[%s] %s: %s\n", time_str, msg->username, msg->content);
            }
            printf("=== End of history ===\n\n");
            return;
        }
    }
    
    // If no conversation found
    print_no_history(target);
} 