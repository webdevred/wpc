#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "filesystem.h"
#include "gui.h"
#include "lightdm.h"
#include "monitors.h"

#include "config.h"
#include "feh.h"
#include "wallpaper.h"
#include <execinfo.h>
#include <glib.h>
#include <gtk/gtk.h>

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
        set_wallpapers();
        exit(0);
    }

    g_log_set_handler("Gtk", G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
                      custom_log_handler, NULL);

    int sock[2];
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sock) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(sock[1]);
        seteuid(geteuid());
        char buffer[1024];
        char *delimimeter;
        while (1) {
            ssize_t bytes = read(sock[0], buffer, sizeof(buffer) - 1);

            buffer[bytes] = '\0';

            delimimeter = strchr(buffer, ' ');

            *delimimeter = '\0';

            char *monitor_name = buffer;
            char *wallpaper_path = delimimeter + 1;

            printf("LightDM: Wallpaper %s Monitor %s\n", wallpaper_path,
                   monitor_name);
            fflush(stdout);
            Monitor *monitor = get_monitor(monitor_name);
            Wallpaper wallpaper = get_wallpaper(wallpaper_path);

            lightdm_set_background(&wallpaper, monitor);
        }

        close(sock[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(sock[0]);

        seteuid(getuid());

        int status = initialize_application(argc, argv, sock);

        close(sock[1]);
        wait(NULL);

        return status;
    }
}
