// Copyright 2025 webdevred

#include <cjson/cJSON.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>

#define DM_CONFIG_PAYLOAD
#include "wpc/lightdm.h"
#include "wpc/lightdm_helper_payload.h"
#include "wpc/wallpaper_transformation.h"

#include "wpc/wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

static void format_dst_filename(gchar **dst_filename) {
    unsigned long len, inSpace, i, j, k;
    gchar *str, *extension_dot;
    inSpace = 0;
    str = *dst_filename;
    i = 0;
    j = 0;
    len = strlen(str);

    extension_dot = strrchr(str, '.');
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

    for (k = 0; k < j; k++) {
        if (str[k] == ' ') {
            str[k] = '-';
        }
    }
}

static int scale_image(Wallpaper *src_image, char *dst_image_path,
                       Monitor monitor, BgMode bg_mode) {
    MagickWand *wand = NULL;

    wand = NewMagickWand();
    MagickReadImage(wand, src_image->path);

    if (bg_mode != BG_MODE_TILE ||
        !transform_wallpaper_tiled(&wand, &monitor)) {
        transform_wallpaper(&wand, &monitor, bg_mode, NULL);
    }
    MagickWriteImages(wand, dst_image_path, MagickTrue);

    wand = DestroyMagickWand(wand);

    return 0;
}

static void setup_child_auto_exit(void) { prctl(PR_SET_PDEATHSIG, SIGTERM); }

static gboolean stdout_callback(GIOChannel *source, GIOCondition condition,
                                gpointer user_data) {
    gchar *line;
    GError *error;
    (void)user_data, (void)condition;
    line = NULL;
    error = NULL;
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
    gchar buffer[1024];
    gsize bytes_read;
    GError *error;
    (void)user_data, (void)condition;

    bytes_read = 0;
    error = NULL;
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
    GMainLoop *loop;
    printf("Child process exited with status %d\n", status);
    g_spawn_close_pid(pid);
    loop = (GMainLoop *)user_data;
    g_main_loop_quit(loop);
}

static void spawn_process_and_handle_io(char **argv, const char *payload) {
    gint in_fd, out_fd, err_fd;
    GPid child_pid;
    GError *error;
    GIOChannel *out_channel;
    GIOChannel *err_channel;
    GMainLoop *loop;
    error = NULL;

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

    out_channel = g_io_channel_unix_new(out_fd);
    err_channel = g_io_channel_unix_new(err_fd);

    g_io_channel_set_encoding(out_channel, NULL, NULL);
    g_io_channel_set_encoding(err_channel, NULL, NULL);

    g_io_add_watch(out_channel, G_IO_IN | G_IO_HUP, stdout_callback, NULL);
    g_io_add_watch(err_channel, G_IO_IN | G_IO_HUP, stderr_callback, NULL);

    loop = g_main_loop_new(NULL, FALSE);
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
    gchar *payload;
    gchar *argv[] = {WPC_HELPER_PATH, NULL};
    char *tmp_file_path, *dst_file_path;
    char base_dir[] = "/usr/share/backgrounds/wpc/versions";
    char *storage_directory =
        g_strdup_printf("%s/%dx%d", base_dir, monitor->width, monitor->height);
    char *new_filename = g_path_get_basename(wallpaper->path);

    format_dst_filename(&new_filename);

    tmp_file_path =
        g_strdup_printf("%s/.wpc_%s.png", g_get_home_dir(), new_filename);
    dst_file_path =
        g_strdup_printf("%s/%s.png", storage_directory, new_filename);

    if (scale_image(wallpaper, tmp_file_path, *monitor, bg_mode) != 0) {
        fflush(stderr);
        g_free(tmp_file_path);
        g_error("Failed to scale image");
    }

    payload = NULL;
    dump_payload(&payload, CONFIG_FILE, tmp_file_path, dst_file_path,
                 monitor->name);

    spawn_process_and_handle_io(argv, payload);

    g_free(storage_directory);
    g_free(tmp_file_path);
    g_free(dst_file_path);
    g_free(payload);
}
