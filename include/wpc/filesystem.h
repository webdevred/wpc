#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <gtk/gtk.h>
#pragma clang diagnostic pop

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

typedef struct {
    WallpaperArray *wallpapers;
    guint current_wallpaper;
} WallpaperQueue;

extern void free_wallpapers(WallpaperArray *arr);

extern WallpaperQueue *new_wallpaper_queue(gchar *source_directory);

extern void free_wallpaper_queue(WallpaperQueue *queue);

extern gchar *next_wallpaper_in_queue(WallpaperQueue *queue);

extern WallpaperArray *list_wallpapers(char *source_directory);
