#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include <wand/MagickWand.h>

#include "filesystem.h"
#include "wallpaper_struct.h"

int set_resolution(Wallpaper *wallpaper) {
    MagickWandGenesis();

    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper->path) == MagickFalse) {
        fprintf(stderr, "Failed to read image: %s\n", wallpaper->path);
        return 1;
    }

    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);

    wallpaper->width = width;
    wallpaper->height = height;

    DestroyMagickWand(wand);
    MagickWandTerminus();
    return 0;
}

extern Wallpaper *list_wallpapers(gchar *source_directory,
                                  int *number_of_images) {
    Wallpaper *wallpaper_array = NULL;
    int wallpaper_array_size = 0;
    DIR *dir;
    struct dirent *file;

    dir = opendir(source_directory);
    if (dir) {
        while ((file = readdir(dir)) != NULL) {
            char *filename = file->d_name;

            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                continue;
            }

            wallpaper_array =
                realloc(wallpaper_array,
                        (wallpaper_array_size + 1) * sizeof(Wallpaper));
            if (!wallpaper_array) {
                closedir(dir);
            }

            Wallpaper *wallpaper = &wallpaper_array[wallpaper_array_size];

            wallpaper_array[wallpaper_array_size].path =
                malloc(strlen(source_directory) * sizeof(char *) +
                       strlen(filename) * sizeof(char *) + sizeof(char *));
            if (wallpaper_array[wallpaper_array_size].path) {
                strcpy(wallpaper->path, source_directory);
                strcat(wallpaper->path, filename);
                set_resolution(wallpaper);
                printf("Image: %s Res %dx%d", wallpaper->path, wallpaper->width,
                       wallpaper->height);
                wallpaper_array_size++;
            }
        }
        closedir(dir);

        wallpaper_array = realloc(wallpaper_array, (wallpaper_array_size + 1) *
                                                       sizeof(Wallpaper));
    }
    wallpaper_array[wallpaper_array_size].path = NULL;

    *number_of_images = wallpaper_array_size;

    return wallpaper_array;
}

extern Wallpaper get_wallpaper(gchar *wallpaper_path) {
    Wallpaper wallpaper;

    wallpaper.path = wallpaper_path;

    set_resolution(&wallpaper);

    return wallpaper;
}
