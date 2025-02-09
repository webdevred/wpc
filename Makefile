SHELL := /bin/bash
CC := gcc

WPC_HELPER ?= 1

WPC_INSTALL_DIR := /usr/local/bin
WPC_HELPER_INSTALL_DIR := /usr/local/libexec/wpc
WPC_HELPER_PATH := $(WPC_HELPER_INSTALL_DIR)/lightdm_helper

COMMON_CFLAGS := -Wall -Wextra -std=c23 -g3
WPC_CFLAGS := $(COMMON_CFLAGS) $(shell pkg-config --cflags gtk4 glib-2.0 MagickWand libcjson) \
              -DWPC_HELPER_PATH="\"$(WPC_HELPER_PATH)\""
WPC_LDFLAGS := $(shell pkg-config --libs gtk4 glib-2.0 x11 xrandr MagickWand imlib2 libcjson)

HELPER_CFLAGS := $(COMMON_CFLAGS)
HELPER_LDFLAGS :=

SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BC_DIR := bc_files

WPC_SRCS := $(wildcard $(SRC_DIR)/*.c)
WPC_SRCS := $(filter-out $(SRC_DIR)/wpc_lightdm_helper.c $(SRC_DIR)/lightdm.c, $(WPC_SRCS))

ifeq ($(WPC_HELPER), 1)
    WPC_CFLAGS += -DENABLE_HELPER
    WPC_SRCS += $(SRC_DIR)/lightdm.c
    HELPER_SRCS := $(SRC_DIR)/wpc_lightdm_helper.c $(SRC_DIR)/common.c
else
    HELPER_SRCS :=
endif

WPC_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(WPC_SRCS))
HELPER_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(HELPER_SRCS))

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
	$(CC) $(if $(filter $(HELPER_SRCS), $<),$(HELPER_CFLAGS),$(WPC_CFLAGS)) \
        -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(BC_DIR) $(WPC_INSTALL_DIR) $(WPC_HELPER_INSTALL_DIR):
	mkdir -p $@

install: all | $(WPC_INSTALL_DIR) $(WPC_HELPER_INSTALL_DIR)
	install -m 0111 wpc $(WPC_INSTALL_DIR)/
ifeq ($(WPC_HELPER), 1)
	install -o root -g root -m 4711 wpc_lightdm_helper $(WPC_HELPER_INSTALL_DIR)/lightdm_helper
endif

clean:
	rm -rf $(BUILD_DIR) $(BC_DIR) $(TARGETS)

bc: | $(BC_DIR)
	$(foreach SRC,$(WPC_SRCS),$(CC) -emit-llvm -c $(SRC) -o $(patsubst $(SRC_DIR)/%.c,$(BC_DIR)/%.bc,$(SRC));)

ccls:
	echo clang > .ccls
	echo -n -I$(INCLUDE_DIR) >> .ccls
	echo -n "$(WPC_CFLAGS) $(HELPER_CFLAGS) $(WPC_LDFLAGS) $(HELPER_LDFLAGS)" | tr ' ' '\n' | sort -u >> .ccls
