#include <cjson/cJSON.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <wand/MagickWand.h>
#include <wand/magick-image.h>

#define DM_CONFIG_PAYLOAD
#include "common.h"
#include "config.h"
#include "lightdm.h"
#include "rendering_region.h"

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

static int scale_image(Wallpaper *src_image, char *dst_image_path,
                       Monitor monitor, BgMode bg_mode) {
#define ThrowWandException(wand)                                               \
    {                                                                          \
        char *description;                                                     \
        ExceptionType severity;                                                \
        description = MagickGetException(wand, &severity);                     \
        (void)fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(),             \
                      description);                                            \
        description = (char *)MagickRelinquishMemory(description);             \
        exit(-1);                                                              \
    }

    MagickWand *wallpaper_wand = NULL;
    PixelWand *color = NULL;
    MagickWand *output_wand = NULL;
    MagickBooleanType status;

    MagickWandGenesis();

    wallpaper_wand = NewMagickWand();
    status = MagickReadImage(wallpaper_wand, src_image->path);
    if (status == MagickFalse) ThrowWandException(wallpaper_wand);

    monitor.horizontal_position = 0;
    monitor.vertical_position = 0;
    
    RenderingRegion *rr =
        create_rendering_region(wallpaper_wand, &monitor, bg_mode);

    status = MagickResizeImage(wallpaper_wand, rr->width, rr->height,
                               LanczosFilter, 1.0);
    if (status == MagickFalse) ThrowWandException(wallpaper_wand);

    status = MagickCropImage(wallpaper_wand, rr->width, rr->height, rr->src_x,
                             rr->src_y);
    if (status == MagickFalse) ThrowWandException(wallpaper_wand);

    color = NewPixelWand();
    PixelSetColor(color, "#ff0000");

    output_wand = NewMagickWand();
    status =
      MagickNewImage(output_wand, monitor.width, monitor.height, color);
    if (status == MagickFalse) ThrowWandException(output_wand);

    status = MagickCompositeImage(output_wand, wallpaper_wand, OverCompositeOp,
                                  rr->monitor_x, rr->monitor_y);
    if (status == MagickFalse) ThrowWandException(output_wand);

    status = MagickWriteImages(output_wand, dst_image_path, MagickTrue);
    if (status == MagickFalse) ThrowWandException(output_wand);

    free(rr);

    output_wand = DestroyMagickWand(output_wand);
    wallpaper_wand = DestroyMagickWand(wallpaper_wand);
    color = DestroyPixelWand(color);

    MagickWandTerminus();
    return 0;
}

static void setup_child_auto_exit() { prctl(PR_SET_PDEATHSIG, SIGTERM); }

static gboolean stdout_callback(GIOChannel *source, GIOCondition condition,
                                gpointer user_data) {
    (void)user_data, (void)condition;
    gchar *line = NULL;
    GError *error = NULL;
    if (g_io_channel_read_line(source, &line, NULL, NULL, &error) ==
            G_IO_STATUS_NORMAL &&
        line) {
        g_info("%s", line);
        g_free(line);
        return TRUE;
    }
    return FALSE;
}

static gboolean stderr_callback(GIOChannel *source, GIOCondition condition,
                                gpointer user_data) {
    (void)user_data, (void)condition;
    gchar buffer[1024];
    gsize bytes_read = 0;
    GError *error = NULL;
    if (g_io_channel_read_chars(source, buffer, sizeof(buffer) - 1, &bytes_read,
                                &error) == G_IO_STATUS_NORMAL &&
        bytes_read > 0) {
        buffer[bytes_read] = '\0';
        fprintf(stderr, "stderr: %s", buffer);
        return TRUE;
    }
    return FALSE;
}

static void child_exit_callback(GPid pid, gint status, gpointer user_data) {
    printf("Child process exited with status %d\n", status);
    g_spawn_close_pid(pid);
    GMainLoop *loop = (GMainLoop *)user_data;
    g_main_loop_quit(loop);
}

static void spawn_process_and_handle_io(char **argv, const char *payload) {
    gint in_fd, out_fd, err_fd;
    GPid child_pid;
    GError *error = NULL;

    if (!g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                                  (GSpawnChildSetupFunc)setup_child_auto_exit,
                                  NULL, &child_pid, &in_fd, &out_fd, &err_fd,
                                  &error)) {
        fprintf(stderr, "Failed to spawn process: %s\n", error->message);
        g_error_free(error);
        return;
    }

    write(in_fd, payload, strlen(payload));
    close(in_fd);

    GIOChannel *out_channel = g_io_channel_unix_new(out_fd);
    GIOChannel *err_channel = g_io_channel_unix_new(err_fd);

    g_io_channel_set_encoding(out_channel, NULL, NULL);
    g_io_channel_set_encoding(err_channel, NULL, NULL);

    g_io_add_watch(out_channel, G_IO_IN | G_IO_HUP, stdout_callback, NULL);
    g_io_add_watch(err_channel, G_IO_IN | G_IO_HUP, stderr_callback, NULL);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_child_watch_add(child_pid, child_exit_callback, loop);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    g_io_channel_unref(out_channel);
    g_io_channel_unref(err_channel);
}

static void dump_payload(char **payload, const char *config_file_path,
                         const char *tmp_file_path, const char *dst_file_path,
                         const char *monitor_name) {

    cJSON *payload_json = cJSON_CreateObject();

    cJSON_AddStringToObject(payload_json, "configFilePath", config_file_path);
    if (config_file_path == NULL) {
        goto end;
    }

    cJSON_AddStringToObject(payload_json, "tmpFilePath", tmp_file_path);
    if (config_file_path == NULL) {
        goto end;
    }

    cJSON_AddStringToObject(payload_json, "dstFilePath", dst_file_path);
    if (config_file_path == NULL) {
        goto end;
    }

    cJSON_AddStringToObject(payload_json, "monitorName", monitor_name);
    if (config_file_path == NULL) {
        goto end;
    }

    *payload = cJSON_PrintUnformatted(payload_json);
end:
    cJSON_Delete(payload_json);
    return;
}

extern void lightdm_set_background(Wallpaper *wallpaper, Monitor *monitor,
                                   BgMode bg_mode) {
    char base_dir[] = "/usr/share/backgrounds/wpc/versions";
    char *storage_directory =
        g_strdup_printf("%s/%dx%d", base_dir, monitor->width, monitor->height);
    char *new_filename = g_path_get_basename(wallpaper->path);

    format_dst_filename(&new_filename);

    char *tmp_file_path =
        g_strdup_printf("%s/.wpc_%s.png", g_get_home_dir(), new_filename);
    char *dst_file_path =
        g_strdup_printf("%s/%s.png", storage_directory, new_filename);

    if (scale_image(wallpaper, tmp_file_path, *monitor, bg_mode) != 0) {
        fflush(stderr);
        g_free(tmp_file_path);
        g_error("Failed to scale image");
    }

    gchar *argv[] = {WPC_HELPER_PATH, NULL};

    char *payload = NULL;
    dump_payload(&payload, CONFIG_FILE, tmp_file_path, dst_file_path,
                 monitor->name);

    spawn_process_and_handle_io(argv, payload);

    g_free(storage_directory);
    g_free(tmp_file_path);
    g_free(dst_file_path);
    g_free(payload);
}

extern void lightdm_get_backgrounds(char **primary_monitor,
                                    char **other_monitor) {
    char **config = NULL;
    int lines = 0;

    if (lightdm_parse_config(&config, &lines) != 0) {
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
