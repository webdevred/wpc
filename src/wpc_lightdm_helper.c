// Copyright 2025 webdevred

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wpc/common.h"
#include "wpc/lightdm_helper_payload.h"

#define BUFFER_SIZE 1024

#define MAX_LINE_LENGTH 1024

static int lightdm_parse_config(char ***config_ptr, gulong *lines_ptr) {
    char **temp, **config;
    guint lines;
    char line[MAX_LINE_LENGTH];
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Error opening configuration file");
        return -1;
    }

    lines = 0;
    config = *config_ptr;

    if (config) {
        for (guint i = 0; i < *lines_ptr; i++) {
            free(config[i]);
        }
        free(config);
        config = NULL;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        temp = realloc(config, (lines + 1) * sizeof(char *));
        if (temp == NULL) {
            perror("Error reallocating memory");
            fclose(file);
            for (guint i = 0; i < lines; i++) {
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
            for (guint i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return -1;
        }
        lines++;
    }

    fclose(file);

    if (lines == 0) {
        config = malloc(sizeof(char *));
        if (config == NULL) {
            perror("Error allocating memory for empty config");
            return -1;
        }
        config[0] = strdup("");
        if (config[0] == NULL) {
            perror("Error allocating memory for empty line");
            free(config);
            return -1;
        }
        lines = 1;
    }

    *config_ptr = config;
    *lines_ptr = lines;
    return 0;
}

static void copy_file(const char *src, const char *dst) {
    int fd_in, fd_out;
    off_t len, ret;
    struct stat stat;
    fd_in = open(src, O_RDONLY);
    if (fd_in == -1) {
        exit(EXIT_FAILURE);
    }

    if (fstat(fd_in, &stat) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    len = stat.st_size;

    fd_out = open(dst, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd_out == -1) {
        exit(EXIT_FAILURE);
    }

    do {
        ret = copy_file_range(fd_in, NULL, fd_out, NULL, (size_t)len, 0);
        if (ret == -1) {
            perror("copy_file_range");
            exit(EXIT_FAILURE);
        }

        len -= ret;
    } while (len > 0 && ret > 0);

    close(fd_in);
    close(fd_out);
}

static gboolean is_this_line_monitor(char *line, const char *monitor_name) {
    char *first_part, *second_part, *last_char;
    char *str = strdup(line);
    gboolean found = FALSE;
    char *delimiter = strstr(str, ": ");
    if (!delimiter) goto end;

    *delimiter = '\0';
    first_part = str;
    second_part = delimiter + 2;

    if (strcmp(first_part, "[monitor") != 0) {
        goto end;
    }

    last_char = &second_part[strlen(second_part) - 1];
    if (*last_char == ']') {
        *last_char = '\0';
    } else {
        goto end;
    }

    if (strcmp(second_part, monitor_name) == 0) found = TRUE;

end:
    free(str);
    return found;
}

static const char bg_key[] = "background=";

static int set_background(const char *scaled_wallpaper_path,
                          const char *dst_wallpaper_path,
                          const char *monitor_name) {
    gboolean found_monitor, updating_monitor;
    gulong lines, bg_key_len;
    char **config, *storage_directory;
    DIR *dir;
    bg_key_len = strlen(bg_key);
    storage_directory = dirname(strdup(dst_wallpaper_path));
    dir = opendir(storage_directory);
    if (dir) {
        closedir(dir);
    } else if (errno == ENOENT) {
        if (create_parent_dirs(dst_wallpaper_path, 0775) != 0) {
            fprintf(stderr, "Failed to create storage directory");
            return 1;
        }
    }

    copy_file(scaled_wallpaper_path, dst_wallpaper_path);

    config = NULL;
    lines = 0;

    found_monitor = FALSE;
    updating_monitor = FALSE;

    if (lightdm_parse_config(&config, &lines) == 0) {
        FILE *file = fopen(CONFIG_FILE, "w");
        if (file == NULL) {
            fprintf(stderr, "Error opening configuration file for writing");
            free(storage_directory);
            for (guint i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return 1;
        }

        for (guint i = 0; i < lines; i++) {
            char *line = config[i];

            if (strcmp(line, "") != 0) {
                if (updating_monitor &&
                    strncmp(bg_key, line, bg_key_len) == 0) {
                    fprintf(file, "background=%s\n", dst_wallpaper_path);
                } else {
                    if (is_this_line_monitor(line, monitor_name)) {
                        updating_monitor = TRUE;
                        found_monitor = TRUE;
                    } else if (line[0] == '[') {
                        updating_monitor = FALSE;
                    }

                    fprintf(file, "%s\n", line);
                }
            }
            free(config[i]);
        }

        if (!found_monitor) {
            fprintf(file, "[monitor: %s]\nbackground=%s\n", monitor_name,
                    dst_wallpaper_path);
        }

        free(config);
        fflush(file);

        fclose(file);

        printf("successfully edited background for LightDM");
    }
    fflush(stdout);
    return 0;
}

static gboolean load_payload(const char *payload, char **config_file_path,
                             char **tmp_file_path, char **dst_file_path,
                             char **monitor_name) {
    cJSON *config_file_path_json, *payload_json, *tmp_file_path_json,
        *dst_file_path_json, *monitor_name_json;
    gboolean failed;
    payload_json = cJSON_Parse(payload);

    if (!payload_json) {
        fprintf(stderr, "JSON parsing error\n");
        return TRUE;
    }

    failed = FALSE;

    config_file_path_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "configFilePath");
    if (cJSON_IsString(config_file_path_json) &&
        config_file_path_json->valuestring) {
        *config_file_path = strdup(config_file_path_json->valuestring);
    } else {
        fprintf(stderr, "Failed to parse configFilePath\n");
        failed = TRUE;
    }

    tmp_file_path_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "tmpFilePath");
    if (cJSON_IsString(tmp_file_path_json) && tmp_file_path_json->valuestring) {
        *tmp_file_path = strdup(tmp_file_path_json->valuestring);
    } else {
        fprintf(stderr, "Failed to parse tmpFilePath\n");
        failed = TRUE;
    }

    dst_file_path_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "dstFilePath");
    if (cJSON_IsString(dst_file_path_json) && dst_file_path_json->valuestring) {
        *dst_file_path = strdup(dst_file_path_json->valuestring);
    } else {
        fprintf(stderr, "Failed to parse dstFilePath\n");
        failed = TRUE;
    }

    monitor_name_json =
        cJSON_GetObjectItemCaseSensitive(payload_json, "monitorName");
    if (cJSON_IsString(monitor_name_json) && monitor_name_json->valuestring) {
        *monitor_name = strdup(monitor_name_json->valuestring);
    } else {
        fprintf(stderr, "Failed to parse monitorName\n");
        failed = TRUE;
    }

    cJSON_Delete(payload_json);

    // Free allocated memory if parsing failed
    if (failed) {
        free(*config_file_path);
        free(*tmp_file_path);
        free(*dst_file_path);
        free(*monitor_name);
        *config_file_path = NULL;
        *tmp_file_path = NULL;
        *dst_file_path = NULL;
        *monitor_name = NULL;
    }

    return failed;
}

extern int main(int argc, char **argv) {
    char buffer[BUFFER_SIZE];
    ssize_t count;

    char *config_file_path;
    char *tmp_wallpaper_path;
    char *dst_wallpaper_path;
    char *monitor_name;

    int status;
    (void)argc, (void)argv;
    status = 1;

    count = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

    buffer[count] = '\0';

    load_payload(buffer, &config_file_path, &tmp_wallpaper_path,
                 &dst_wallpaper_path, &monitor_name);

    status =
        set_background(tmp_wallpaper_path, dst_wallpaper_path, monitor_name);

    fflush(stderr);

    goto end;

end:
    free(config_file_path);
    free(tmp_wallpaper_path);
    free(dst_wallpaper_path);
    return status;
}
