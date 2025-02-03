#include "config.h"
#include <cjson/cJSON.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE "/home/debbie/.config/wpc/settings.json"

void free_config(gpointer data) {
    Config *config = (Config *)data;
    int number_of_monitors = config->number_of_monitors;
    free(config->source_directory);
    for (int i = 0; i < number_of_monitors; i++) {
        MonitorBackgroundPair monitor_background_pair =
            config->monitors_with_backgrounds[i];
        free(monitor_background_pair.name);
        free(monitor_background_pair.image_path);
    }
    free(config);
}

Config *load_config() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Error opening configuration file");
        return NULL;
    }

    Config *config = (Config *)malloc(sizeof(Config));
    if (!config) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }
    config->monitors_with_backgrounds = NULL;
    config->number_of_monitors = 0;

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
        config->source_directory = strdup(directory_json->valuestring);
    }

    cJSON *monitors_json = cJSON_GetObjectItemCaseSensitive(
        settings_json, "monitorsWithBackgrounds");
    cJSON *monitor_background_json;
    int number_of_monitors = 0;

    cJSON_ArrayForEach(monitor_background_json, monitors_json) {
        config->monitors_with_backgrounds =
            realloc(config->monitors_with_backgrounds,
                    (number_of_monitors + 1) * sizeof(MonitorBackgroundPair));

        if (!config->monitors_with_backgrounds) {
            perror("Memory reallocation failed");
            cJSON_Delete(settings_json);
            return NULL;
        }

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

// NOTE: Returns a heap allocated string, you are required to free it after use.
void dump_config(Config *config) {
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

    while (config->monitors_with_backgrounds != NULL) {
        cJSON *monitor_background_json = cJSON_CreateObject();

        MonitorBackgroundPair *monitor_background_pair =
            config->monitors_with_backgrounds;

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
    printf("%s", string);
}
