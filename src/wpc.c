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

#include "feh.h"
#include "filelist.h"
#include "winwidget.h"
#include "signals.h"
#include "wallpaper.h"

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "-b") == 0) {
      char backgrounds[] = "/mnt/HDD/backgrounds/bakgrund.jpg";
      init_x_and_imlib();
      filelist = gib_list_add_front(filelist, feh_file_new(backgrounds));
      filelist_len = gib_list_length(filelist);
      feh_wm_set_bg_filelist(BG_MODE_FILL);
      exit(0);
    }

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

        return initialize_application(argc, argv, sock);

        close(sock[1]);
        wait(NULL);
    }
}
