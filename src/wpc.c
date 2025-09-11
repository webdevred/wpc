// Copyright 2025 webdevred

#include <glib.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#include "wpc/gui.h"
#include "wpc/monitors.h"
#include "wpc/options.h"
#include "wpc/wallpaper.h"

#include "wpc/wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

static int set_backgrounds_and_exit(void) {
    Config *config = load_config();
    MonitorArray *monitor_array;
    WallpaperQueue *queue;
    monitor_array = list_monitors(TRUE);
    queue = new_wallpaper_queue(config->source_directory);
    set_wallpapers(config, queue, monitor_array);
    free_wallpaper_queue(queue);
    free_config(config);
    free_monitors(monitor_array);
    return 0;
}

volatile static bool terminate = FALSE;

static void handle_termination(int signal) {
    (void)signal;
    terminate = TRUE;
}

static int fork_and_exit(void) {
    pid_t pid = fork();
    Config *config;
    MonitorArray *monitor_array;
    WallpaperQueue *queue;
    if (pid == 0) {
        signal(SIGINT, handle_termination);
        signal(SIGTERM, handle_termination);

        config = load_config();
        init_x11();
        monitor_array = list_monitors(TRUE);
        MagickWandGenesis();
        queue = new_wallpaper_queue(config->source_directory);
        while (!terminate) {
            set_wallpapers(config, queue, monitor_array);
            sleep(350);
        }
        free_wallpaper_queue(queue);

        free_config(config);
        free_monitors(monitor_array);
        MagickWandTerminus();
    }
    return 0;
}

extern int main(int argc, char **argv) {
    int status;
    Options *options = malloc(sizeof(Options));
    parse_options(argv, options);

    if (options->action != DAEMON_SET_BACKGROUNDS) {
        init_x11();
        MagickWandGenesis();
    }

    switch (options->action) {
    case SET_BACKGROUNDS_AND_EXIT:
        status = set_backgrounds_and_exit();
        break;
    case DAEMON_SET_BACKGROUNDS:
        status = fork_and_exit();
        break;
    default:
        status = initialize_application(argc, argv);
        break;
    }

    if (options->action != DAEMON_SET_BACKGROUNDS) {
        MagickWandTerminus();
    }

    free(options);

    return status;
}
