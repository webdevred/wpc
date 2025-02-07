#include "common.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int mkpath(char *file_path, mode_t mode) {
    assert(file_path && *file_path);
    for (char *p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}

static int set_background(char *scaled_wallpaper_path, char *dst_wallpaper_path,
                          bool primary_monitor) {
    char *storage_directory = dirname(dst_wallpaper_path);
    DIR *dir = opendir(storage_directory);
    if (dir) {
        closedir(dir);
    } else if (errno == ENOENT) {
        if (mkdir(storage_directory, 0775) != 0) {
            printf("Failed to create storage directory");
            fflush(stdout);
            return 1;
        }
    }

    rename(scaled_wallpaper_path, dst_wallpaper_path);

    char **config = NULL;
    int lines = 0;
    bool found = false;
    if (parse_config(&config, &lines) == 0) {
        FILE *file = fopen(CONFIG_FILE, "w");
        if (file == NULL) {
            printf("Error opening configuration file for writing");
            fflush(stdout);
            free(storage_directory);
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
                if (primary_monitor &&
                    strcmp(key, "background") == 0) {
                    fprintf(file, "%s=%s\n", key, dst_wallpaper_path);
                    found = true;
                } else if (! primary_monitor &&
                           strcmp(key, "other-monitors-logo") == 0) {
                    fprintf(file, "%s=%s\n", key, dst_wallpaper_path);
                    found = true;
                } else {
                    *delimiter = '=';
                    fprintf(file, "%s\n", line);
                }
            } else {
                fprintf(file, "%s\n", line);
            }
            free(config[i]);
        }

        if (!found) {
            char *key = primary_monitor ? "background" : "other-monitors-logo";
            fprintf(file, "%s=%s\n", key, dst_wallpaper_path);
        }

        free(config);
        fflush(file);

        fclose(file);

        printf("successfully edited background for LightDM");
    }
    fflush(stdout);
    return 0;
}

extern int main(int argc, char **argv) {
    (void)argc, (void)argv;
    char buffer[BUFFER_SIZE];
    ssize_t count;

    char *tmp_wallpaper_path;
    char *dst_wallpaper_path;
    bool monitor_primary;

    int status = 1;

    count = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (count > 0) {
        buffer[count] = '\0';

        char *delimiter;
        delimiter = strchr(buffer, ' ');

        *delimiter = '\0';

        tmp_wallpaper_path = strdup(buffer);
        dst_wallpaper_path = strdup(delimiter + 1);
        delimiter = strchr(dst_wallpaper_path, ' ');
        *delimiter = '\0';
        monitor_primary = strcmp(delimiter + 1, "true") == 0 ? true : false;

        status = set_background(tmp_wallpaper_path, dst_wallpaper_path,
                                monitor_primary);
    }
    return status;
}
