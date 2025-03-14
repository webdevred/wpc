#pragma once

#include <gtk/gtk.h>

typedef struct {
    size_t width, height;
    gchar *path;
    GtkFlowBoxChild *flow_child;
} Wallpaper;

typedef struct {
    Wallpaper *data;
    gushort amount_allocated;
    gushort amount_used;
} WallpaperArray;

extern void free_wallpapers(WallpaperArray *arr);

extern WallpaperArray *list_wallpapers(char *source_directory);
