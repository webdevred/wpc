#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <wand/MagickWand.h>

#include "wpc.h"
#include "lightdm_common.h"
#include "lightdm.h"
#include "monitors.h"
#include "resolution_scaling.h"


extern void lightdm_get_backgrounds(char **primary_monitor,
                                    char **other_monitor) {
    char **config = NULL;
    int lines = 0;

    if (parse_config(&config, &lines) != 0) {
        fprintf(stderr, "Failed to parse config file.\n");
        return;
    }

    for (int i = 0; i < lines; i++) {
        char *line = config[i];
        char *delimiter = strchr(line, '=');

        if (delimiter) {
            *delimiter = '\0';
            char *key = line;
            char *value = delimiter + 1;

            if (strcmp(key, "background") == 0) {
                *primary_monitor = strdup(value);
            } else if (strcmp(key, "other-monitors-logo") == 0) {
                *other_monitor = strdup(value);
            }
        }
        free(config[i]);
    }
    free(config);
}

static void format_dst_filename(gchar **dst_filename) {
    gchar *str = *dst_filename;
    int i = 0, j = 0;
    int len = strlen(str);
    int inSpace = 0;

    gchar *extension_dot = strrchr(str, '.');
    *extension_dot = '\0';

    while (i < len) {
        if (isspace(str[i]) || str[i] == '-') {
            if (!inSpace) {
                str[j++] = ' ';
                inSpace = 1;
            }
        } else if (i > 50) {
            break;
        } else {
            str[j++] = str[i];
            inSpace = 0;
        }
        i++;
    }

    str[j] = '\0';

    for (int k = 0; k < j; k++) {
        if (str[k] == ' ') {
            str[k] = '-';
        }
    }
}

static int scale_image(Wallpaper *src_image, char *dst_image_path, Monitor *monitor) {
#define ThrowWandException(wand)                                               \
    {                                                                          \
        char *description;                                                     \
                                                                               \
        ExceptionType severity;                                                \
                                                                               \
        description = MagickGetException(wand, &severity);                     \
        (void)fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(),             \
                      description);                                            \
        description = (char *)MagickRelinquishMemory(description);             \
        exit(-1);                                                              \
    }

    MagickBooleanType status;

    MagickWand *magick_wand;

    MagickWandGenesis();
    magick_wand = NewMagickWand();
    status = MagickReadImage(magick_wand, src_image->path);
    if (status == MagickFalse) ThrowWandException(magick_wand);

    int aspect_image_height =
        scale_height(src_image->width, src_image->height, monitor->width);
    int vertical_offset = (int)((aspect_image_height - monitor->height) / 2);

    if (vertical_offset < 0) vertical_offset = 0;

    MagickResetIterator(magick_wand);
    if (MagickNextImage(magick_wand) != MagickFalse) {
        MagickResizeImage(magick_wand, monitor->width, aspect_image_height,
                          LanczosFilter, 1.0);
        MagickCropImage(magick_wand, monitor->width, monitor->height, 0,
                        vertical_offset);
    }

    status = MagickWriteImages(magick_wand, dst_image_path, MagickTrue);
    if (status == MagickFalse) ThrowWandException(magick_wand);
    magick_wand = DestroyMagickWand(magick_wand);
    MagickWandTerminus();
    return (0);
}

extern void lightdm_set_background(Wallpaper *wallpaper, Monitor *monitor) {
    gchar *new_filename = g_path_get_basename(wallpaper->path);
    format_dst_filename(&new_filename);
    gchar *tmp_file_path =
        g_strdup_printf("%s/.wpc_%s.png", g_get_home_dir(), new_filename);
    if (scale_image(wallpaper, tmp_file_path, monitor) != 0) {
        fflush(stderr);
        g_free(tmp_file_path);
        logprintf(ERROR, "Failed to scale image");
    }

    gchar *args = serialize_args(monitor->name, wallpaper->path);

    gchar *argv[] = {"/usr/local/libexec/wpc_helper", NULL};
    GError *error = NULL;
    GPid child_pid;
    gint in_fd, out_fd, err_fd;

    if (!g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL, NULL, &child_pid, &in_fd, &out_fd,
                                  &err_fd, &error)) {
        g_print("Failed to spawn helper: %s\n", error->message);
        g_error_free(error);
        free(args);
        return;
    }

    write(in_fd, args, strlen(args));

    free(args);

    char buffer[256];
    ssize_t count = read(out_fd, buffer, sizeof(buffer) - 1);
    if (count > 0) {
        buffer[count] = '\0';
        g_print("Response from helper: %s\n", buffer);
    } else {
        g_print("No response from helper.\n");
    }

    close(in_fd);
    close(out_fd);
    close(err_fd);

    g_spawn_close_pid(child_pid);
}
