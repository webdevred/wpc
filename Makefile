SHELL := /bin/bash

CC = gcc

CFLAGS = -Wall -Wextra -std=c23 -g3 \
         $(shell pkg-config --cflags gtk4) \
         $(shell pkg-config --cflags glib-2.0) \
         $(shell pkg-config --cflags MagickWand) \
         $(shell pkg-config --cflags libcjson) \
         -D_POSIX_C_SOURCE=200809L

LDFLAGS = $(shell pkg-config --libs gtk4) \
          $(shell pkg-config --libs glib-2.0) \
          $(shell pkg-config --libs x11) \
          $(shell pkg-config --libs xrandr) \
          $(shell pkg-config --libs MagickWand) \
          $(shell pkg-config --libs imlib2) \
          $(shell pkg-config --libs libcjson)

WPC_INSTALL_DIR = /usr/local/bin
WPC_HELPER_INSTALL_DIR = /usr/local/libexec/wpc

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
BC_DIR = bc_files

WPC_SRCS = $(filter-out $(SRC_DIR)/wpc_lightdm_helper.c, $(wildcard $(SRC_DIR)/*.c))
HELPER_SRCS = $(SRC_DIR)/wpc_lightdm_helper.c $(SRC_DIR)/lightdm_common.c $(SRC_DIR)/imagemagick.c

WPC_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(WPC_SRCS))
HELPER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(HELPER_SRCS))

TARGETS = wpc wpc_lightdm_helper

.PHONY: all clean install bc ccls

all: $(TARGETS)

wpc: $(WPC_OBJS)
	$(CC) $(WPC_OBJS) $(LDFLAGS) -o $@

wpc_lightdm_helper: $(HELPER_OBJS)
	$(CC) $(HELPER_OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

wpc_install_dir:
	mkdir -p $(WPC_INSTALL_DIR)

lightdm_helper_install_dir:
	mkdir -p $(WPC_HELPER_INSTALL_DIR)
	chmod +x $(WPC_HELPER_INSTALL_DIR)

install: wpc_install_dir lightdm_helper_install_dir
	mv wpc $(WPC_INSTALL_DIR)/wpc
	chmod 0111 $(WPC_INSTALL_DIR)/wpc
	mv wpc_lightdm_helper $(WPC_HELPER_INSTALL_DIR)/lightdm_helper
	chown "root:root" $(WPC_HELPER_INSTALL_DIR)/lightdm_helper
	chmod 0001 $(WPC_HELPER_INSTALL_DIR)/lightdm_helper
	chmod u+s $(WPC_HELPER_INSTALL_DIR)/lightdm_helper

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)

BC_FILES := $(patsubst $(SRC_DIR)/%.c, $(BC_DIR)/%.bc, $(wildcard $(SRC_DIR)/*.c))

$(BC_DIR):
	mkdir -p $(BC_DIR)

$(BC_DIR)/%.bc: $(SRC_DIR)/%.c | $(BC_DIR)
	$(CC) -I $(INCLUDE_DIR) $(CFLAGS) -c -emit-llvm -o $@ $<

bc: $(BC_FILES)

ccls:
	echo clang > .ccls
	echo -n -Iinclude >> .ccls
	echo -n "$(CFLAGS) $(LDFLAGS)" | sed 's/ /\n/g' | sort | uniq >> .ccls
