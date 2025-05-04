#include "gui.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#include "monitors.h"
#include "options.h"
#include "wallpaper.h"
#include <glib.h>

#include "wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

extern int set_backgrounds_and_exit() {
    Config *config = load_config();
    MonitorArray *monitor_array = list_monitors(true);
    set_wallpapers(config, monitor_array);
    free_config(config);
    free_monitors(monitor_array);
    return 0;
}

volatile bool terminate = false;

static void handle_termination(int signal) {
    (void)signal;
    terminate = true;
}

extern int fork_and_exit() {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, handle_termination);
        signal(SIGTERM, handle_termination);

        Config *config = load_config();
        init_x();
        MonitorArray *monitor_array = list_monitors(true);

        while (!terminate) {
            set_wallpapers(config, monitor_array);
            sleep(5000);
        }

        free_config(config);
        free_monitors(monitor_array);
    }
    return 0;
}

extern int main(int argc, char **argv) {
    Options *options = malloc(sizeof(Options));
    parse_options(argv, options);

    MagickWandGenesis();

    init_x();

    int status;

    switch (options->action) {
    case SET_BACKGROUNDS_AND_EXIT:
        status = set_backgrounds_and_exit();
        break;
    case DAEMON_SET_BACKGROUNDS:
        status = fork_and_exit();
        status = 0;
        break;
    case START_GUI:
        status = initialize_application(argc, argv);
    }

    MagickWandTerminus();

    return status;
}
