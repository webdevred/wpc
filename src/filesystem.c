#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "wpc.h"
#include "filesystem.h"
#include "imagemagick.h"

extern Wallpaper *list_wallpapers(gchar *source_directory,
                                  int *number_of_images) {
    Wallpaper *wallpaper_array = NULL;
    int wallpaper_array_size = 0;
    DIR *dir;
    struct dirent *file;

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

        wallpaper_array[wallpaper_array_size].path =
            malloc(strlen(source_directory) * sizeof(char *) +
                   strlen(filename) * sizeof(char *) + sizeof(char *));
        if (wallpaper_array[wallpaper_array_size].path) {
            strcpy(wallpaper->path, source_directory);
            strcat(wallpaper->path, filename);
            set_resolution(wallpaper);
            wallpaper_array_size++;
        }
    }
    closedir(dir);

    wallpaper_array = realloc(wallpaper_array,
                              (wallpaper_array_size + 1) * sizeof(Wallpaper));
    wallpaper_array[wallpaper_array_size].path = NULL;

    *number_of_images = wallpaper_array_size;

    return wallpaper_array;
}
