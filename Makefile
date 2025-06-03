# Compiler settings
ifeq ($(CROSS),win32)
    # Cross-compile for 32-bit Windows
    CC = i686-w64-mingw32-gcc
    CFLAGS = -Wall -Wextra -I./include
    LDFLAGS = -lws2_32
    RM = rm -f
    MKDIR = mkdir -p
    RMDIR = rm -rf
    EXE_EXT = .exe
else ifeq ($(CROSS),win64)
    # Cross-compile for 64-bit Windows
    CC = x86_64-w64-mingw32-gcc
    CFLAGS = -Wall -Wextra -I./include
    LDFLAGS = -lws2_32
    RM = rm -f
    MKDIR = mkdir -p
    RMDIR = rm -rf
    EXE_EXT = .exe
else ifeq ($(OS),Windows_NT)
    # Native Windows build
    CC = gcc
    CFLAGS = -Wall -Wextra -I./include
    LDFLAGS = -lws2_32
    RM = del /Q /F
    MKDIR = mkdir
    RMDIR = rmdir /S /Q
    EXE_EXT = .exe
else
    # Native Linux build
    CC = gcc
    CFLAGS = -Wall -Wextra -I./include
    LDFLAGS = -pthread
    RM = rm -f
    MKDIR = mkdir -p
    RMDIR = rm -rf
    EXE_EXT =
endif

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
CLIENT_SRC = $(SRC_DIR)/client/main.c \
             $(SRC_DIR)/client/message.c \
             $(SRC_DIR)/client/conversation.c \
             $(SRC_DIR)/client/display.c

SERVER_SRC = $(SRC_DIR)/server/main.c \
             $(SRC_DIR)/server/client_handler.c \
             $(SRC_DIR)/server/message.c \
             $(SRC_DIR)/server/storage.c

# Object files
CLIENT_OBJ = $(CLIENT_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SERVER_OBJ = $(SERVER_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executables
CLIENT_BIN = $(BIN_DIR)/client$(EXE_EXT)
SERVER_BIN = $(BIN_DIR)/server$(EXE_EXT)

# Libraries
CLIENT_LIB = libclient.a
SERVER_LIB = libserver.a

# Targets
all: directories $(CLIENT_BIN) $(SERVER_BIN)

# Cross-compilation targets
win32: CROSS=win32
win32: all

win64: CROSS=win64
win64: all

directories:
	$(MKDIR) $(OBJ_DIR)
	$(MKDIR) $(OBJ_DIR)/client
	$(MKDIR) $(OBJ_DIR)/server
	$(MKDIR) $(BIN_DIR)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(CLIENT_OBJ) $(SERVER_OBJ)
	$(RM) $(CLIENT_BIN) $(SERVER_BIN)
	$(RM) $(CLIENT_LIB) $(SERVER_LIB)
	$(RMDIR) $(OBJ_DIR)
	$(RMDIR) $(BIN_DIR)

.PHONY: all clean directories win32 win64