#include "config.h"
#include <cjson/cJSON.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE ".config/wpc/settings.json"

static char *get_config_file() {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s/%s", home, CONFIG_FILE);
}

static void free_monitor_background_pair(MonitorBackgroundPair *pair) {
    if (pair) {
        free(pair->name);
        pair->name = NULL;
        free(pair->image_path);
        pair->image_path = NULL;
    }
}

extern void free_config(Config *config) {
    if (config) {
        if (config->monitors_with_backgrounds) {
            for (int i = 0; i < config->number_of_monitors; i++) {
                free_monitor_background_pair(
                    &config->monitors_with_backgrounds[i]);
            }
            free(config->monitors_with_backgrounds);
        }

        free(config);
    }
}

static void get_xdg_pictures_dir(Config *config) {
    const char *xdg_pictures_dir =
        g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    strcpy(config->source_directory, xdg_pictures_dir);
    printf("%s", config->source_directory);
}

static void append_slash_path(Config *config) {
    size_t length = strlen(config->source_directory);

    if (config->source_directory[length - 1] != '/') {
        char *new_str = malloc(length + 2);
        if (!new_str) {
            return;
        }

        strcpy(new_str, config->source_directory);
        strcat(new_str, "/");

        strcpy(config->source_directory, new_str);
    }
}

extern void update_source_directory(Config *config, char* new_src_dir) {
     strcpy(config->source_directory, new_src_dir);
}

extern Config *load_config() {
    FILE *file = fopen(get_config_file(), "r");
    Config *config = (Config *)malloc(sizeof(Config));

    config->monitors_with_backgrounds = NULL;
    config->number_of_monitors = 0;

    if (file == NULL) {
        get_xdg_pictures_dir(config);
        append_slash_path(config);
        return config;
    }

    if (!config) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    size_t capacity = 255;
    char *file_content = malloc(capacity);
    if (!file_content) {
        perror("Memory allocation failed");
        free(config);
        fclose(file);
        return NULL;
    }
    file_content[0] = '\0';

    size_t file_size = 0;
    char line[100];

    while (fgets(line, sizeof(line), file)) {
        size_t line_len = strlen(line);
        if (file_size + line_len + 1 > capacity) {
            capacity *= 2;
            char *temp = realloc(file_content, capacity);
            if (!temp) {
                perror("Memory reallocation failed");
                free(file_content);
                free_config(config);
                fclose(file);
                return NULL;
            }
            file_content = temp;
        }
        strcat(file_content, line);
        file_size += line_len;
    }
    fclose(file);

    cJSON *settings_json = cJSON_Parse(file_content);
    free(file_content);

    if (!settings_json) {
        fprintf(stderr, "JSON parsing error\n");
        free_config(config);
        return NULL;
    }

    cJSON *directory_json =
        cJSON_GetObjectItemCaseSensitive(settings_json, "sourceDirectoryPath");
    if (cJSON_IsString(directory_json) && directory_json->valuestring) {
      strcpy(config->source_directory, directory_json->valuestring);
    } else {
        get_xdg_pictures_dir(config);
    }

    append_slash_path(config);

    cJSON *monitors_json = cJSON_GetObjectItemCaseSensitive(
        settings_json, "monitorsWithBackgrounds");
    cJSON *monitor_background_json;
    int number_of_monitors = 0;

    cJSON_ArrayForEach(monitor_background_json, monitors_json) {
        MonitorBackgroundPair *temp =
            realloc(config->monitors_with_backgrounds,
                    (number_of_monitors + 1) * sizeof(MonitorBackgroundPair));
        if (!temp) {
            perror("Memory reallocation failed");
            cJSON_Delete(settings_json);
            free_config(config);
            return NULL;
        }
        config->monitors_with_backgrounds = temp;

        MonitorBackgroundPair *monitor_background_pair =
            &config->monitors_with_backgrounds[number_of_monitors];

        cJSON *monitor_name_json =
            cJSON_GetObjectItemCaseSensitive(monitor_background_json, "name");
        cJSON *background_json = cJSON_GetObjectItemCaseSensitive(
            monitor_background_json, "imagePath");

        if (cJSON_IsString(monitor_name_json) &&
            cJSON_IsString(background_json)) {
            monitor_background_pair->name =
                strdup(monitor_name_json->valuestring);
            monitor_background_pair->image_path =
                strdup(background_json->valuestring);
        }

        number_of_monitors++;
    }

    config->number_of_monitors = number_of_monitors;
    cJSON_Delete(settings_json);
    return config;
}

extern void dump_config(Config *config) {
    FILE *file = fopen(get_config_file(), "w");
    if (file == NULL) {
        perror("Error opening configuration file");
        exit(1);
    }
    char *string = NULL;
    cJSON *monitors_with_backgrounds = NULL;

    cJSON *settings = cJSON_CreateObject();

    if (cJSON_AddStringToObject(settings, "sourceDirectoryPath",
                                config->source_directory) == NULL) {
        goto end;
    }

    monitors_with_backgrounds =
        cJSON_AddArrayToObject(settings, "monitorsWithBackgrounds");
    if (monitors_with_backgrounds == NULL) {
        goto end;
    }

    int number_of_monitors = config->number_of_monitors;
    for (int i = 0; i < number_of_monitors; i++) {
        cJSON *monitor_background_json = cJSON_CreateObject();
        MonitorBackgroundPair *monitor_background_pair =
            &config->monitors_with_backgrounds[i];

        if (cJSON_AddStringToObject(monitor_background_json, "name",
                                    monitor_background_pair->name) == NULL) {
            goto end;
        }

        if (cJSON_AddStringToObject(monitor_background_json, "imagePath",
                                    monitor_background_pair->image_path) ==
            NULL) {
            goto end;
        }

        cJSON_AddItemToArray(monitors_with_backgrounds,
                             monitor_background_json);
    }

    string = cJSON_Print(settings);
    if (string == NULL) {
        fprintf(stderr, "Failed to print monitor.\n");
    }

end:
    cJSON_Delete(settings);
    fwrite(string, sizeof(char), strlen(string), file);
    fflush(file);
    fclose(file);
    free(string);
}
