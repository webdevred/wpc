SHELL := /bin/bash
CC := gcc

WPC_HELPER ?= 1

WPC_INSTALL_DIR := /usr/local/bin
WPC_HELPER_INSTALL_DIR := /usr/local/libexec/wpc
WPC_HELPER_PATH := $(WPC_HELPER_INSTALL_DIR)/lightdm_helper

COMMON_CFLAGS := -Wall -Wextra -std=c23 -g3
COMMON_LDFLAGS := $(shell pkg-config --libs libcjson)

WPC_CFLAGS := $(COMMON_CFLAGS) $(shell pkg-config --cflags gtk4 glib-2.0 MagickWand libcjson) -DWPC_HELPER_PATH="\"$(WPC_HELPER_PATH)\""
WPC_LDFLAGS := $(COMMON_LDFLAGS) $(shell pkg-config --libs gtk4 glib-2.0 x11 xrandr MagickWand)

HELPER_CFLAGS := $(COMMON_CFLAGS)
HELPER_LDFLAGS := $(COMMON_LDFLAGS)

SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BC_DIR := bc_files

WPC_SRCS := $(wildcard $(SRC_DIR)/*.c)
WPC_SRCS := $(filter-out $(SRC_DIR)/wpc_lightdm_helper.c $(SRC_DIR)/lightdm.c, $(WPC_SRCS))

ifeq ($(WPC_HELPER), 1)
    WPC_SRCS += $(SRC_DIR)/lightdm.c
    WPC_CFLAGS += -DWPC_ENABLE_HELPER
    HELPER_SRCS := $(SRC_DIR)/wpc_lightdm_helper.c $(SRC_DIR)/common.c
endif

WPC_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(WPC_SRCS))
HELPER_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(HELPER_SRCS))

WPC_BC := $(patsubst $(SRC_DIR)/%.c, $(BC_DIR)/%.bc, $(WPC_SRCS))
HELPER_BC := $(patsubst $(SRC_DIR)/%.c, $(BC_DIR)/%.bc, $(HELPER_SRCS))
BC_FILES := $(WPC_BC) $(HELPER_BC)

TARGETS := wpc
ifeq ($(WPC_HELPER), 1)
    TARGETS += wpc_lightdm_helper
endif

.PHONY: all clean install bc ccls

.SILENT: ccls

all: $(TARGETS)

wpc: $(WPC_OBJS)
	$(CC) $(WPC_OBJS) $(WPC_LDFLAGS) -o $@

ifeq ($(WPC_HELPER), 1)
wpc_lightdm_helper: $(HELPER_OBJS)
	$(CC) $(HELPER_OBJS) $(HELPER_LDFLAGS) -o $@
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(if $(filter $(HELPER_SRCS), $<),$(HELPER_CFLAGS),$(WPC_CFLAGS)) -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

$(BC_DIR)/%.bc: $(SRC_DIR)/%.c | $(BC_DIR)
	$(CC) $(if $(filter $(HELPER_SRCS), $<),$(HELPER_CFLAGS),$(WPC_CFLAGS)) -I$(INCLUDE_DIR) -I/usr/include/glib-2.0 -c -emit-llvm $< -o $@

$(BUILD_DIR) $(BC_DIR) $(WPC_INSTALL_DIR) $(WPC_HELPER_INSTALL_DIR):
	mkdir -p $@

install: all | $(WPC_INSTALL_DIR) $(WPC_HELPER_INSTALL_DIR)
	install -m 0111 wpc $(WPC_INSTALL_DIR)/
ifeq ($(WPC_HELPER), 1)
	install -o root -g root -m 4711 wpc_lightdm_helper $(WPC_HELPER_INSTALL_DIR)/lightdm_helper
endif

clean:
	rm -rf $(BUILD_DIR) $(BC_DIR) $(TARGETS)

bc: $(BC_FILES)

ccls:
	echo clang > .ccls
	echo -n -I$(INCLUDE_DIR) >> .ccls
	echo -n "$(WPC_CFLAGS) $(HELPER_CFLAGS) $(WPC_LDFLAGS) $(HELPER_LDFLAGS)" | tr ' ' '\n' | sort -u >> .ccls
