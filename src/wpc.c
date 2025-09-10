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

extern int set_backgrounds_and_exit() {
    Config *config = load_config();
    MonitorArray *monitor_array = list_monitors(TRUE);
    WallpaperQueue *queue = new_wallpaper_queue(config->source_directory);
    set_wallpapers(config, queue, monitor_array);
    free_wallpaper_queue(queue);
    free_config(config);
    free_monitors(monitor_array);
    return 0;
}

volatile bool terminate = FALSE;

static void handle_termination(int signal) {
    (void)signal;
    terminate = TRUE;
}

extern int fork_and_exit() {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, handle_termination);
        signal(SIGTERM, handle_termination);

        Config *config = load_config();
        init_x11();
        MonitorArray *monitor_array = list_monitors(TRUE);
        MagickWandGenesis();
        WallpaperQueue *queue = new_wallpaper_queue(config->source_directory);
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
    Options *options = malloc(sizeof(Options));
    parse_options(argv, options);

    int status;

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
    case START_GUI:
        status = initialize_application(argc, argv);
    }

    if (options->action != DAEMON_SET_BACKGROUNDS) {
        MagickWandTerminus();
    }

    free(options);

    return status;
}
