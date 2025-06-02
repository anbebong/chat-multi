CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -static -pthread

# Source directories
CLIENT_SRC = src/client
SERVER_SRC = src/server

# Client source files
CLIENT_SOURCES = $(CLIENT_SRC)/main.c \
                 $(CLIENT_SRC)/message.c \
                 $(CLIENT_SRC)/conversation.c \
                 $(CLIENT_SRC)/display.c

# Server source files
SERVER_SOURCES = $(SERVER_SRC)/main.c \
                 $(SERVER_SRC)/message.c \
                 $(SERVER_SRC)/storage.c \
                 $(SERVER_SRC)/client_handler.c

# Object files
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)

# Tên các file static library
SERVER_LIB = libserver.a
CLIENT_LIB = libclient.a

# Tên các file thực thi
SERVER = server.out
CLIENT = client.out

all: $(SERVER) $(CLIENT)

# Build static libraries
$(SERVER_LIB): $(SERVER_OBJECTS)
	$(AR) rcs $@ $^

$(CLIENT_LIB): $(CLIENT_OBJECTS)
	$(AR) rcs $@ $^

# Link statically
$(SERVER): $(SERVER_LIB)
	$(CC) $(LDFLAGS) -o $@ $^

$(CLIENT): $(CLIENT_LIB)
	$(CC) $(LDFLAGS) -o $@ $^

# Client object files
$(CLIENT_SRC)/main.o: $(CLIENT_SRC)/main.c include/client.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_SRC)/message.o: $(CLIENT_SRC)/message.c include/client.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_SRC)/conversation.o: $(CLIENT_SRC)/conversation.c include/client.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_SRC)/display.o: $(CLIENT_SRC)/display.c include/client.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Server object files
$(SERVER_SRC)/main.o: $(SERVER_SRC)/main.c include/server.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_SRC)/message.o: $(SERVER_SRC)/message.c include/server.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_SRC)/storage.o: $(SERVER_SRC)/storage.c include/server.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_SRC)/client_handler.o: $(SERVER_SRC)/client_handler.c include/server.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER) $(CLIENT) $(SERVER_LIB) $(CLIENT_LIB) $(SERVER_OBJECTS) $(CLIENT_OBJECTS)
clean-deps:
	rm -f $(SERVER_OBJECTS) $(CLIENT_OBJECTS)
.PHONY: all clean clean-deps