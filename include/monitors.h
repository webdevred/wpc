#pragma once
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <filesystem.h>
#include <glib.h>
#include <stdbool.h>

typedef struct {
    gushort width, height;
    gshort left_x, top_y;
    gushort config_id;
    bool belongs_to_config;
    bool primary;
    gchar *name;
    Wallpaper *wallpaper;
} Monitor;

typedef struct {
    Wallpaper *data;
    gushort amount_allocated;
    gushort amount_used;
} MonitorArray;

extern void init_disp(Display **disp, Window *root);

extern void free_monitors(MonitorArray *arr);

extern MonitorArray *list_monitors(const bool virtual_monitors);

extern Monitor *get_monitor(const gchar *monitor_name);
