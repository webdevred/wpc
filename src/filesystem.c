#include <dirent.h>
#include <glib.h>
#include <magic.h>
#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

static bool check_image(const char *filename) {
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    if (!magic) {
        fprintf(stderr, "Failed to initialize libmagic\n");
        return FALSE;
    }

    magic_load(magic, NULL);

    bool valid = TRUE;
    const char *mime_type = magic_file(magic, filename);
    if (!mime_type || strncmp(mime_type, "image/", 6) != 0) {
        valid = FALSE;
        g_info("File %s has invalid MIME type %s", filename, mime_type);
    }

    magic_close(magic);

    return valid;
}

static bool set_width_and_height(Wallpaper *wallpaper) {
    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper->path) == MagickFalse) {
        g_warning("Failed to read image when setting width and height: %s\n",
                  wallpaper->path);
        DestroyMagickWand(wand);
        MagickWandTerminus();
        return FALSE;
    }

    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);

    wallpaper->width = width;
    wallpaper->height = height;

    DestroyMagickWand(wand);
    return TRUE;
}

extern void free_wallpapers(WallpaperArray *arr) {
    Wallpaper *wallpapers = (Wallpaper *)arr->data;
    unsigned int i;
    for (i = 0; i < arr->amount_used; i++) {
        free(wallpapers[i].path);
    }
    free(wallpapers);
    free(arr);
}

extern WallpaperArray *list_wallpapers(gchar *source_directory) {
    DIR *dir = opendir(source_directory);
    if (!dir) {
        closedir(dir);
        g_warning("dir %s does not exist", source_directory);
        return NULL;
    }

    WallpaperArray *array_wrapper = malloc(sizeof(WallpaperArray));
    if (!array_wrapper) {
        closedir(dir);
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

        wallpaper->flow_child = NULL;

        if (!check_image(wallpaper->path) || !set_width_and_height(wallpaper)) {
            free(wallpaper->path);
            continue;
        }

        amount_used++;
    }

    closedir(dir);

    array_wrapper->amount_allocated = amount_allocated;
    array_wrapper->amount_used = amount_used;

    return array_wrapper;
}
