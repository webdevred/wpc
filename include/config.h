#pragma once

#include <glib.h>

typedef enum {
    BG_MODE_CENTER = 1,
    BG_MODE_FILL,
    BG_MODE_MAX,
    BG_MODE_SCALE,
    BG_MODE_TILE,
    BG_MODE_NOT_SET
} BgMode;

typedef struct {
    gchar *name;
    gchar *image_path;
    BgMode bg_mode;
} MonitorBackgroundPair;

typedef struct {
    gushort number_of_monitors;
    MonitorBackgroundPair *monitors_with_backgrounds;
    gchar *source_directory;
} Config;

extern int lightdm_parse_config(char ***config_ptr, int *lines_ptr);

extern void free_config(Config *config);

extern void update_source_directory(Config *config, const gchar *new_src_dir);

extern Config *load_config(void);

extern void dump_config(Config *config);
