#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "imagemagick.h"
#include "wpc.h"

extern Wallpaper *list_wallpapers(gchar *source_directory,
                                  int *number_of_images) {
    Wallpaper *wallpaper_array = NULL;
    int wallpaper_array_size = 0;
    DIR *dir;
    struct dirent *file;

    unsigned int src_dir_len = strlen(source_directory);

    dir = opendir(source_directory);
    if (!dir) {
        return NULL;
    }
    while ((file = readdir(dir)) != NULL) {
        char *filename = file->d_name;

        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
            continue;
        }

        wallpaper_array = realloc(wallpaper_array, (wallpaper_array_size + 1) *
                                                       sizeof(Wallpaper));
        if (!wallpaper_array) {
            closedir(dir);
            return NULL;
        }

        Wallpaper *wallpaper = &wallpaper_array[wallpaper_array_size];

        if (source_directory[src_dir_len - 1] != '/') {
            snprintf(wallpaper->path, sizeof(wallpaper->path), "%s%s%s",
                     source_directory,
                     (source_directory[src_dir_len - 1] != '/') ? "/" : "",
                     filename);
        }

        set_resolution(wallpaper);
        wallpaper_array_size++;
    }
    closedir(dir);

    *number_of_images = wallpaper_array_size;

    return wallpaper_array;
}
