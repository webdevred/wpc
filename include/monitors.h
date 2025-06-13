#pragma once
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <filesystem.h>
#include <glib.h>

typedef struct {
    gushort width, height;
    gshort left_x, top_y;
    gushort config_id;
    gboolean belongs_to_config;
    gboolean primary;
    gchar *name;
    Wallpaper *wallpaper;
} Monitor;

typedef struct {
    Wallpaper *data;
    gushort amount_allocated;
    gushort amount_used;
} MonitorArray;

extern Display *querying_display;
extern Window querying_root;
extern int querying_depth;

extern Display *rendering_display;
extern Window rendering_root;
extern Visual *rendering_visual;
extern int rendering_depth;
extern Colormap rendering_colormap;
extern Screen *rendering_screen;

extern void init_x11(void);

extern void free_monitors(MonitorArray *arr);

extern MonitorArray *list_monitors(const bool virtual_monitors);

extern Monitor *get_monitor(const gchar *monitor_name);
