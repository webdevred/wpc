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
    gchar *bg_fallback_color;
    gchar *valid_bg_fallback_color;
    BgMode bg_mode;
} ConfigMonitor;

typedef struct {
    gushort number_of_monitors;
    ConfigMonitor *monitors_with_backgrounds;
    gchar *source_directory;
    bool valid_source_directory;
} Config;

extern void init_config_monitor(Config *config, const gchar *monitor_name,
                                const gchar *wallpaper_path,
                                const BgMode bg_mode);

extern void free_config(Config *config);

extern void update_source_directory(Config *config, const gchar *new_src_dir);

extern Config *load_config(void);

extern void dump_config(Config *config);
