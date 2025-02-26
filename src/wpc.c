#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "gui.h"

#include "wallpaper.h"
#include <execinfo.h>
#include <glib.h>

void custom_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                        const gchar *message, gpointer user_data) {
    (void)user_data;
    (void)log_domain;
    if (log_level & (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL)) {
        g_printerr("GTK Warning: %s\n", message);

        void *array[10];
        size_t size = backtrace(array, 10);
        char **symbols = backtrace_symbols(array, size);
        g_printerr("Stack trace:\n");
        for (size_t i = 0; i < size; i++) {
            g_printerr("%s\n", symbols[i]);
        }
        g_free(symbols);
    }
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "-b") == 0) {
        Config *config = load_config();
        set_wallpapers(config);
        free_config(config);
        exit(0);
    }

    g_log_set_handler("Gtk", G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
                      custom_log_handler, NULL);

    return initialize_application(argc, argv);
}
