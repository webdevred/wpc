SHELL := /bin/bash
CC := clang

WPC_HELPER ?= 1
WPC_IMAGEMAGICK_7 ?= 1

WPC_INSTALL_DIR := /usr/local/bin
WPC_HELPER_INSTALL_DIR := /usr/local/libexec/wpc
WPC_HELPER_PATH := $(WPC_HELPER_INSTALL_DIR)/lightdm_helper

COMMON_CFLAGS := -Wall -Wextra -std=gnu11 -g3  $(shell pkg-config --cflags glib-2.0)
COMMON_LDFLAGS := $(shell pkg-config --libs libcjson glib-2.0)

WPC_CFLAGS := $(COMMON_CFLAGS) $(shell pkg-config --cflags gtk4 MagickWand libcjson) -DWPC_HELPER_PATH="\"$(WPC_HELPER_PATH)\""
WPC_LDFLAGS := $(COMMON_LDFLAGS) $(shell pkg-config --libs gtk4 x11 xrandr MagickWand libmagic)

HELPER_CFLAGS := $(COMMON_CFLAGS)
HELPER_LDFLAGS := $(COMMON_LDFLAGS)

SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BC_DIR := bc_files

WPC_SRCS := $(wildcard $(SRC_DIR)/*.c)
WPC_SRCS := $(filter-out $(SRC_DIR)/wpc_lightdm_helper.c $(SRC_DIR)/lightdm.c, $(WPC_SRCS))

ifeq ($(WPC_IMAGEMAGICK_7), 1)
    WPC_CFLAGS += -DWPC_IMAGEMAGICK_7
    WPC_LDFLAGS += -DWPC_IMAGEMAGICK_7
endif

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

.PHONY: all clean install bc iwyu compile_commands

all: $(TARGETS) bc compile_commands

wpc: $(WPC_OBJS)
	$(CC) $(WPC_OBJS) $(WPC_LDFLAGS) -o $(BUILD_DIR)/$@

ifeq ($(WPC_HELPER), 1)
wpc_lightdm_helper: $(HELPER_OBJS)
	$(CC) $(HELPER_OBJS) $(HELPER_LDFLAGS) -o $(BUILD_DIR)/$@
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -MJ $@.json $(if $(filter $(HELPER_SRCS), $<),$(HELPER_CFLAGS),$(WPC_CFLAGS)) -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

$(BC_DIR)/%.bc: $(SRC_DIR)/%.c | $(BC_DIR)
	$(CC) $(if $(filter $(HELPER_SRCS), $<),$(HELPER_CFLAGS),$(WPC_CFLAGS)) -I$(INCLUDE_DIR) -I/usr/include/glib-2.0 -c -emit-llvm $< -o $@

$(BUILD_DIR) $(BC_DIR) $(WPC_INSTALL_DIR) $(WPC_HELPER_INSTALL_DIR):
	mkdir -p $@

install: all | $(WPC_INSTALL_DIR) $(WPC_HELPER_INSTALL_DIR)
	install -m 0111 wpc $(WPC_INSTALL_DIR)/
ifeq ($(WPC_HELPER), 1)
	install -o root -g root -m 4711 wpc_lightdm_helper $(WPC_HELPER_INSTALL_DIR)/lightdm_helper
endif

iwyu:
	@for file in $(WPC_SRCS) $(HELPER_SRCS); do \
		echo "Running iwyu on $$file..."; \
		include-what-you-use -Xiwyu --transitive_includes_only -std=gnu11 $(WPC_CFLAGS) $(WPC_LDFLAGS) -I$(INCLUDE_DIR) $$file; \
	done

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)
	$(MAKE) clean_bc

clean_bc:
	find $(BC_DIR) -type f -name '*.bc' -delete

bc: | $(BC_DIR)
	$(MAKE) $(BC_FILES)

compile_commands.json: $(WPC_OBJS) $(HELPER_OBJS)
	@echo "[" > compile_commands.json
	@find $(BUILD_DIR) -name '*.json' -exec cat {} + | sed '$$s/,$$//' >> compile_commands.json
	@echo "]" >> compile_commands.json

compile_commands: compile_commands.json
