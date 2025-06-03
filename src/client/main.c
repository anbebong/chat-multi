#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "client.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 56001

// Global variables (defined here, declared as extern in client.h)
message_t messages[MAX_MESSAGES];
user_t users[MAX_USERS];
conversation_t conversations[MAX_CONVERSATIONS];
int message_count = 0;
int user_count = 0;
int conversation_count = 0;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
int sockfd;
char my_username[MAX_USERNAME];
char current_recipient[MAX_USERNAME] = "all";

int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    struct sockaddr_in serv_addr;
    pthread_t receive_thread;
    char input_buffer[BUFFER_SIZE];
    
    // Get username
    printf("Enter username: ");
    fgets(my_username, MAX_USERNAME, stdin);
    my_username[strcspn(my_username, "\n")] = 0;
    
    // Get password
    char password[MAX_PASSWORD];
    printf("Enter password: ");
    fgets(password, MAX_PASSWORD, stdin);
    password[strcspn(password, "\n")] = 0;
    
    // Authentication loop
    while (1) {
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            error("Error creating socket");
        }
        
        // Initialize server address
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        
        if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
            error("Invalid address");
        }
        
        // Connect to server
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            error("Connection failed");
        }
        
        // Send username first
        if (send(sockfd, my_username, strlen(my_username), 0) < 0) {
            close(sockfd);
            error("Error sending username");
        }
        send(sockfd, "\n", 1, 0);  // Send newline to mark end of username
        
        // Send password
        if (send(sockfd, password, strlen(password), 0) < 0) {
            close(sockfd);
            error("Error sending password");
        }
        send(sockfd, "\n", 1, 0);  // Send newline to mark end of password

        // Wait for server response
        char response[BUFFER_SIZE];
        int n = recv(sockfd, response, BUFFER_SIZE - 1, 0);
        
        // If connection closed by server (n = 0) or error (n < 0)
        if (n <= 0) {
            close(sockfd);
            // If server closed connection, it means authentication failed
            // Try again with new password
            printf("Authentication failed. Please try again.\n");
            printf("Enter password: ");
            fgets(password, MAX_PASSWORD, stdin);
            password[strcspn(password, "\n")] = 0;
            continue;
        }

        response[n] = '\0';
        
        // Check server response
        if (strncmp(response, "Error:", 6) == 0) {
            // Authentication failed
            printf("%s", response);  // Print error message from server
            
            if (strstr(response, "Invalid password") != NULL) {
                // Password was wrong, ask for password again
                close(sockfd);
                printf("Enter password: ");
                fgets(password, MAX_PASSWORD, stdin);
                password[strcspn(password, "\n")] = 0;
                continue;
            } else if (strstr(response, "User is already connected") != NULL) {
                // User is already online, exit
                close(sockfd);
                error("User is already connected");
            } else {
                // Other error, exit
                close(sockfd);
                error(response);
            }
        } else {
            // Authentication successful
            break;
        }
    }
    
    // Print welcome message
    printf("\n=== Welcome to Chat ===\n");
    printf("Connected as: %s\n", my_username);
    print_help();
    print_message("System", "Connected to chat server", 1, "all");
    
    // Create thread for receiving messages
    if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
        error("Error creating thread");
    }
    
    // Register cleanup handler
    atexit(cleanup);
    
    // Main input loop
    while (1) {
        print_prompt();
        
        if (fgets(input_buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove newline
        input_buffer[strcspn(input_buffer, "\n")] = 0;
        
        // Handle commands
        if (input_buffer[0] == '/') {
            if (strcmp(input_buffer, "/quit") == 0) {
                break;
            } else if (strcmp(input_buffer, "/help") == 0) {
                print_help();
            } else if (strcmp(input_buffer, "/clear") == 0) {
                system("clear");
                printf("=== Chat ===\n");
                printf("Connected as: %s\n\n", my_username);
            } else if (strcmp(input_buffer, "/users") == 0) {
                // Send command to server
                if (send(sockfd, "/users", strlen("/users"), 0) < 0) {
                    error("Error sending command");
                }
                // Don't wait for response, it will be handled by receive_messages thread
                continue;
            } else if (strcmp(input_buffer, "/conv") == 0) {
                print_active_conversations();
            } else if (strncmp(input_buffer, "/history ", 9) == 0) {
                char *target = input_buffer + 9;
                if (strlen(target) > 0) {
                    print_conversation_history(target);
                } else {
                    printf("Please specify a username\n");
                }
            } else if (strncmp(input_buffer, "/to ", 4) == 0) {
                char *target = input_buffer + 4;
                if (strlen(target) > 0) {
                    strcpy(current_recipient, target);
                    printf("Now chatting with %s\n", current_recipient);
                    print_conversation_history(current_recipient);
                } else {
                    printf("Please specify a username\n");
                }
            } else if (strcmp(input_buffer, "/all") == 0) {
                strcpy(current_recipient, "all");
                printf("Switched to public chat\n");
            } else {
                printf("Unknown command. Type /help for available commands.\n");
            }
            continue;
        }
        
        // Send normal message
        if (strlen(input_buffer) > 0) {
            char message[BUFFER_SIZE + MAX_USERNAME * 2];
            snprintf(message, sizeof(message), "MSG|%s|%s|%s", my_username, current_recipient, input_buffer);
            
            // Display our own message immediately
            print_message(my_username, input_buffer, 0, current_recipient);
            
            // Add to conversation history
            add_to_conversation(my_username, input_buffer, current_recipient, 
                              strcmp(current_recipient, "all") != 0);
            
            if (send(sockfd, message, strlen(message), 0) < 0) {
                error("Error sending message");
            }
        }
    }
    
    close(sockfd);

#ifdef _WIN32
    // Cleanup Winsock
    WSACleanup();
#endif
    return 0;
}
