#include "common.h"

#include <cjson/cJSON.h>

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

static int mkpath(char *file_path, mode_t mode) {
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
    char *storage_directory = dirname(strdup(dst_wallpaper_path));
    DIR *dir = opendir(storage_directory);
    if (dir) {
        closedir(dir);
    } else if (errno == ENOENT) {
        if (mkpath(storage_directory, 0775) != 0) {
            fprintf(stderr, "Failed to create storage directory");
            return 1;
        }
    }

    rename(scaled_wallpaper_path, dst_wallpaper_path);

    char **config = NULL;
    int lines = 0;
    bool found = false;
    if (lightdm_parse_config(&config, &lines) == 0) {
        FILE *file = fopen(CONFIG_FILE, "w");
        if (file == NULL) {
            fprintf(stderr, "Error opening configuration file for writing");
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
                if (primary_monitor && strcmp(key, "background") == 0) {
                    fprintf(file, "%s=%s\n", key, dst_wallpaper_path);
                    found = true;
                } else if (!primary_monitor &&
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

static bool load_config(const char *payload, char **config_file_path,
                        char **tmp_file_path, char **dst_file_path,
                        bool *primary_monitor) {
    cJSON *payload_json = cJSON_Parse(payload);

    if (!payload_json) {
        fprintf(stderr, "JSON parsing error\n");
        return 1;
    }

    bool failed = false;

    cJSON *config_file_path_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "configFilePath");
    if (cJSON_IsString(config_file_path_json) &&
        config_file_path_json->valuestring) {
        *config_file_path = strdup(config_file_path_json->valuestring);
    } else {
        failed = true;
    }

    cJSON *tmp_file_path_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "tmpFilePath");
    if (cJSON_IsString(tmp_file_path_json) && tmp_file_path_json->valuestring) {
        *tmp_file_path = strdup(tmp_file_path_json->valuestring);
    } else {
        failed = true;
    }

    cJSON *dst_file_path_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "dstFilePath");
    if (cJSON_IsString(dst_file_path_json) && dst_file_path_json->valuestring) {
        *dst_file_path = strdup(dst_file_path_json->valuestring);
    } else {
        failed = true;
    }

    cJSON *primary_monitor_json = cJSON_GetObjectItemCaseSensitive(
        payload_json, "updateForPrimaryMonitor");
    if (cJSON_IsString(primary_monitor_json) &&
        primary_monitor_json->valuestring) {
        *primary_monitor = strdup(primary_monitor_json->valuestring);
    } else {
        failed = true;
    }

    cJSON_Delete(payload_json);
    return failed;
}

static void free_maybe(char *str) {
    if (str) free(str);
}

extern int main(int argc, char **argv) {
    (void)argc, (void)argv;
    char buffer[BUFFER_SIZE];
    ssize_t count;

    char *config_file_path;
    char *tmp_wallpaper_path;
    char *dst_wallpaper_path;
    bool monitor_primary;

    int status = 1;

    count = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (count > 0) {
        buffer[count] = '\0';

        if (!load_config(buffer, &config_file_path, &tmp_wallpaper_path,
                         &dst_wallpaper_path, &monitor_primary)) {
            fprintf(stderr, "received invalid payload: %s\n", buffer);
            return 1;
        }

        status = set_background(tmp_wallpaper_path, dst_wallpaper_path,
                                monitor_primary);

        free_maybe(config_file_path);
        free_maybe(tmp_wallpaper_path);
        free_maybe(dst_wallpaper_path);
    }
    return status;
}
