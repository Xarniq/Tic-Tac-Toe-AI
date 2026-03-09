# Makefile for Tic-Tac-Toe with Raylib

# Project Name
PROJECT_NAME ?= ttt_game
SERVER_NAME ?= ttt_server

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
EXT_DIR = external
RAYLIB_ROOT = $(EXT_DIR)/raylib
RAYLIB_SRC_DIR = $(RAYLIB_ROOT)/src

# Source Subdirectories
CORE_DIR = $(SRC_DIR)/core
AI_DIR = $(SRC_DIR)/ai
UI_DIR = $(SRC_DIR)/ui
NET_DIR = $(SRC_DIR)/network
SRV_DIR = $(SRC_DIR)/server
PROTOCOL_DIR = $(SRC_DIR)/protocol

# Compiler and Flags
CC ?= gcc
CFLAGS = -std=c99 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I$(INC_DIR) -I$(RAYLIB_SRC_DIR) -g
LDFLAGS = -L$(RAYLIB_SRC_DIR) -lraylib -lm -lpthread -ldl -lrt

# Detect OS/Arch for specific flags
UNAME_S := $(shell uname -s)

# Cross-compilation support
ifdef CROSS_COMPILE
    CC = $(CROSS_COMPILE)gcc
endif

ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lX11
endif

# Source files
CLIENT_SRCS = $(wildcard $(SRC_DIR)/*.c) \
			  $(wildcard $(UI_DIR)/*.c) \
			  $(wildcard $(NET_DIR)/*.c) \
			  $(wildcard $(PROTOCOL_DIR)/*.c) \
			  $(CORE_DIR)/board.c \
			  $(CORE_DIR)/config.c

SERVER_SRCS = $(wildcard $(SRV_DIR)/*.c) \
			  $(wildcard $(CORE_DIR)/*.c) \
			  $(wildcard $(AI_DIR)/*.c) \
			  $(wildcard $(PROTOCOL_DIR)/*.c)

# Object files
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SRCS))
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SRCS))

# Targets
.PHONY: all clean run raylib server client

all: raylib $(BIN_DIR)/$(PROJECT_NAME) $(BIN_DIR)/$(SERVER_NAME)

client: $(BIN_DIR)/$(PROJECT_NAME)
server: $(BIN_DIR)/$(SERVER_NAME)

# Build Raylib
raylib:
	@if [ ! -d "$(RAYLIB_SRC_DIR)" ]; then \
		echo "[raylib] sources not found under $(RAYLIB_SRC_DIR)"; \
		echo "Run 'git submodule update --init --recursive $(RAYLIB_ROOT)' first."; \
		exit 1; \
	fi
	$(MAKE) -C $(RAYLIB_SRC_DIR) CC=$(CC) RAYLIB_LIBTYPE=STATIC

# Build Game Client
$(BIN_DIR)/$(PROJECT_NAME): $(CLIENT_OBJS) | $(BIN_DIR)
	$(CC) $(CLIENT_OBJS) -o $@ $(LDFLAGS)

# Build Server
$(BIN_DIR)/$(SERVER_NAME): $(SERVER_OBJS) | $(BIN_DIR)
	$(CC) $(SERVER_OBJS) -o $@ -lm -lpthread

# Pattern rule for each directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	$(MAKE) -C $(RAYLIB_SRC_DIR) clean

# Run (Native only)
run: all
	./$(BIN_DIR)/$(PROJECT_NAME)

run-server: server
	./$(BIN_DIR)/$(SERVER_NAME)
