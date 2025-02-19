#pragma once

#include "monitors.h"
#include <glib.h>

typedef enum {
    BG_MODE_TILE = 0,
    BG_MODE_CENTER,
    BG_MODE_SCALE,
    BG_MODE_FILL,
    BG_MODE_MAX,
    BG_MODE_LAST
} BgMode;

typedef struct {
    char *name;
    char *image_path;
    BgMode bg_mode;
} MonitorBackgroundPair;

typedef struct {
    ushort number_of_monitors;
    MonitorBackgroundPair *monitors_with_backgrounds;
    char *source_directory;
} Config;

extern int lightdm_parse_config(char ***config_ptr, int *lines_ptr);

extern void free_config(Config *config);

extern void update_source_directory(Config *config, const char *new_src_dir);

extern Config *load_config(void);

extern void dump_config(Config *config);
