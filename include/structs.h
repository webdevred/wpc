#pragma once

#include <glib.h>
#include <gtk/gtk.h>

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

typedef struct {
    gushort width;
    gushort height;
    gchar *path;
    GtkFlowBoxChild *flow_child;
} Wallpaper;

typedef struct {
    gushort width;
    gushort height;
    gushort vertical_position;
    gushort horizontal_position;
    gushort config_id;
    bool primary;
    gchar *name;
    Wallpaper *wallpaper;
} Monitor;

typedef struct {
    void *data;
    gushort amount_allocated;
    gushort amount_used;
} ArrayWrapper;

