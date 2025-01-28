#include <dirent.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libexif/exif-data.h>
#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "wallpaper_struct.h"

void extract_dimensions(ExifEntry *entry, void *data) {
    Wallpaper *wallpaper = (Wallpaper *)data;
    char buffer[1024];
    exif_entry_get_value(entry, buffer, sizeof(buffer));

    if (entry->tag == EXIF_TAG_IMAGE_WIDTH) {
        wallpaper->width = atoi(buffer);
    } else if (entry->tag == EXIF_TAG_IMAGE_LENGTH) {
        wallpaper->height = atoi(buffer);
    }
}

int set_resolution(Wallpaper *wallpaper) {
    ExifData *exifData = exif_data_new_from_file(wallpaper->path);

    if (!exifData) {
        return 1;
    }

    // Iterate through EXIF tags
    for (int i = 0; i < EXIF_IFD_COUNT; i++) {
        ExifContent *content = exifData->ifd[i];
        if (content) {
            exif_content_foreach_entry(content, extract_dimensions, wallpaper);
        }
    }

    exif_data_unref(exifData);
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
