#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "imagemagick.h"
#include "wpc.h"

extern void free_wallpapers(ArrayWrapper *arr) {
    Wallpaper *wallpapers = (Wallpaper *)arr->data;
    unsigned int i;
    for (i = 0; i < arr->amount_used; i++) {
        free(wallpapers[i].path);
    }
    free(wallpapers);
    free(arr);
}

extern ArrayWrapper *list_wallpapers(gchar *source_directory) {
    ArrayWrapper *array_wrapper = malloc(sizeof(ArrayWrapper));
    if (!array_wrapper) {
        return NULL;
    }

    array_wrapper->data = malloc(8 * sizeof(Wallpaper));
    if (!array_wrapper->data) {
        free(array_wrapper);
        return NULL;
    }

    Wallpaper *wallpaper_array = (Wallpaper *)array_wrapper->data;
    gushort amount_used = 0;
    gushort amount_allocated = 8;

    DIR *dir = opendir(source_directory);
    if (!dir) {
        free(array_wrapper->data);
        free(array_wrapper);
        return NULL;
    }

    gushort src_dir_len = strlen(source_directory);
    bool slash_needed = source_directory[src_dir_len - 1] != '/';
    src_dir_len += slash_needed ? 1 : 0;

    struct dirent *file;
    while ((file = readdir(dir)) != NULL) {
        gchar *filename = file->d_name;

        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
            continue;
        }

        if (amount_used >= amount_allocated) {
            amount_allocated += 8;
            wallpaper_array = realloc(array_wrapper->data,
                                      amount_allocated * sizeof(Wallpaper));
            if (!wallpaper_array) {
                free(array_wrapper->data);
                free(array_wrapper);
                closedir(dir);
                return NULL;
            }
            array_wrapper->data = wallpaper_array;
        }

        Wallpaper *wallpaper = &wallpaper_array[amount_used];

        gushort path_size = src_dir_len + strlen(filename) + 1;
        wallpaper->path = malloc(path_size);
        if (!wallpaper->path) {
            free(array_wrapper->data);
            free(array_wrapper);
            closedir(dir);
            return NULL;
        }

        snprintf(wallpaper->path, path_size, "%s%s%s", source_directory,
                 slash_needed ? "/" : "", filename);

        set_resolution(wallpaper->path, wallpaper);

        amount_used++;
    }

    closedir(dir);

    array_wrapper->amount_allocated = amount_allocated;
    array_wrapper->amount_used = amount_used;

    return array_wrapper;
}
