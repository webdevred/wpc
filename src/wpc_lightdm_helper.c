#include "lightdm_common.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256




static int set_background(char *scaled_wallpaper_path, Monitor *monitor) {
    char base_dir[] = "/usr/share/backgrounds/wpc/versions";
    char *storage_directory =
        g_strdup_printf("%s/%dx%d", base_dir, monitor->width, monitor->height);

    char *dst_file_path =
        g_strdup_printf("%s/%s.png", storage_directory, scaled_wallpaper_path);

    DIR *dir = opendir(storage_directory);
    if (dir) {
        closedir(dir);
    } else if (errno == ENOENT) {
        if (g_mkdir_with_parents(storage_directory, 0775) != 0) {
            perror("Failed to create storage directory");
            fflush(stderr);
            g_free(dst_file_path);
            return 1;
        }
    }

    if (rename(scaled_wallpaper_path, dst_file_path) != 0) {
        perror("Failed to move scaled image to destination");
        return 1;
    }

    char **config = NULL;
    int lines = 0;
    if (parse_config(&config, &lines) == 0) {
        FILE *file = fopen(CONFIG_FILE, "w");
        if (file == NULL) {
            perror("Error opening configuration file for writing");
            fflush(stderr);
            g_free(storage_directory);
            g_free(dst_file_path);
            for (int i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return 1;
        }

        for (int i = 0; i < lines; i++) {
            char *line = config[i];
            char *delimiter = strchr(line, '=');

            if (delimiter) {
                *delimiter = '\0';
                char *key = line;
                if (monitor->primary == true &&
                    strcmp(key, "background") == 0) {
                    fprintf(file, "%s=%s\n", key, dst_file_path);
                } else if (!monitor->primary &&
                           strcmp(key, "other-monitors-logo") == 0) {
                    fprintf(file, "%s=%s\n", key, dst_file_path);
                } else {
                    *delimiter = '=';
                    fprintf(file, "%s\n", line);
                }
            } else {
                fprintf(file, "%s\n", line);
            }
            free(config[i]);
        }
        free(config);
        fflush(file);

        fclose(file);

        printf("successfully edited background for LightDM");
    }
    return 0;
}

extern int main(int argc, char **argv) {
    (void)argc, (void)argv;
    char buffer[BUFFER_SIZE];
    ssize_t count;

    count = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (count > 0) {
        buffer[count] = '\0';



        printf("%s %s %s", lightdm_config_file, monitor_name, wallpaper_path);
        Monitor *monitor = get_monitor(monitor_name);

        set_background(wallpaper_path, monitor);
    }
    return 0;
}
