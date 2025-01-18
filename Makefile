SHELL := /bin/bash

# Compiler
CC = gcc

# Flags for compiling and linking
CFLAGS = -Wall -Wextra -std=c11 -g $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags glib-2.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs x11) $(shell pkg-config --libs xrandr)

# Project structure
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Source files and corresponding object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Target executable
TARGET = wpc

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Rule to build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Phony targets
.PHONY: all clean
