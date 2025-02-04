#include "imagemagick.h"
#include "monitors.h"
#include <dirent.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE "/etc/lightdm/slick-greeter.conf"
#define MAX_LINE_LENGTH 1024

static int parse_config(char ***config_ptr, int *lines_ptr) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Error opening configuration file");
        return -1;
    }

    int lines = 0;
    char **config = NULL;
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        // Strip newline character
        line[strcspn(line, "\n")] = '\0';

        char **temp = realloc(config, (lines + 1) * sizeof(char *));
        if (temp == NULL) {
            perror("Error reallocating memory");
            fclose(file);
            for (int i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return -1;
        }
        config = temp;

        config[lines] = strdup(line);
        if (config[lines] == NULL) {
            perror("Error allocating memory for line");
            fclose(file);
            for (int i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return -1;
        }
        lines++;
    }

    fclose(file);

    *config_ptr = config;
    *lines_ptr = lines;
    return 0;
}

extern void lightdm_get_backgrounds(char **primary_monitor,
                                    char **other_monitor) {
    char **config = NULL;
    int lines = 0;

    if (parse_config(&config, &lines) != 0) {
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

extern void lightdm_set_background(Wallpaper *wallpaper, Monitor *monitor) {
    gchar base_dir[] = "/usr/share/backgrounds/wpc/versions";
    gchar *storage_directory =
        g_strdup_printf("%s/%dx%d", base_dir, monitor->width, monitor->height);
    gchar *dst_filename = g_path_get_basename(wallpaper->path);
    gchar *dst_file_path =
        g_strdup_printf("%s/%s.png", storage_directory, dst_filename);
    gchar *tmp_file_path =
        g_strdup_printf("%s/.wpc_%s.png", g_get_home_dir(), dst_filename);

    DIR *dir = opendir(storage_directory);
    if (dir) {
        closedir(dir);
    } else if (errno == ENOENT) {
        if (g_mkdir_with_parents(storage_directory, 0775) != 0) {
            perror("Failed to create storage directory");
            g_free(storage_directory);
            g_free(dst_filename);
            g_free(dst_file_path);
            g_free(tmp_file_path);
            return;
        }
    }

    if (scale_image(wallpaper, tmp_file_path, monitor) != 0) {
        fprintf(stderr, "Failed to scale the image\n");
        g_free(storage_directory);
        g_free(dst_filename);
        g_free(dst_file_path);
        g_free(tmp_file_path);
        return;
    }

    if (rename(tmp_file_path, dst_file_path) != 0) {
        perror("Failed to move scaled image to destination");
        unlink(tmp_file_path);
    }

    char **config = NULL;
    int lines = 0;
    if (parse_config(&config, &lines) == 0) {
        FILE *file = fopen(CONFIG_FILE, "w");
        if (file == NULL) {
            perror("Error opening configuration file for writing");
            g_free(storage_directory);
            g_free(dst_filename);
            g_free(dst_file_path);
            g_free(tmp_file_path);
            for (int i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return;
        }

        for (int i = 0; i < lines; i++) {
            char *line = config[i];
            char *delimiter = strchr(line, '=');

            if (delimiter) {
                *delimiter = '\0';
                char *key = line;

                if (strcmp(key, "background") == 0) {
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

        fclose(file);
    }

    g_free(storage_directory);
    g_free(dst_filename);
    g_free(dst_file_path);
    g_free(tmp_file_path);
}
