#include "config.h"
#include <cjson/cJSON.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE ".config/wpc/settings.json"

static gchar *get_config_file() {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s/%s", home, CONFIG_FILE);
}

static void free_monitor_background_pair(ConfigMonitor *pair) {
    if (!pair) return;
    if (pair->name) {
        free(pair->name);
        pair->name = NULL;
    }
    if (pair->image_path) {
        free(pair->image_path);
        pair->image_path = NULL;
    }
}

extern void free_config(Config *config) {
    if (!config) return;

    free(config->source_directory);
    if (config->monitors_with_backgrounds) {
        for (int i = 0; i < config->number_of_monitors; i++) {
            free_monitor_background_pair(&config->monitors_with_backgrounds[i]);
        }
        free(config->monitors_with_backgrounds);
        config->monitors_with_backgrounds = NULL;
    }

    free(config);
    config = NULL;
}

const gchar *bg_mode_to_string(BgMode type) {
    switch (type) {
    case BG_MODE_TILE:
        return "TILE";
    case BG_MODE_CENTER:
        return "CENTER";
    case BG_MODE_SCALE:
        return "SCALE";
    case BG_MODE_MAX:
        return "MAX";
    case BG_MODE_FILL:
        return "FILL";
    default:
        return "";
    }
}

BgMode bg_mode_from_string(const gchar *str) {
    if (strcmp(str, "TILE") == 0)
        return BG_MODE_TILE;
    else if (strcmp(str, "CENTER") == 0)
        return BG_MODE_CENTER;
    else if (strcmp(str, "SCALE") == 0)
        return BG_MODE_SCALE;
    else if (strcmp(str, "MAX") == 0)
        return BG_MODE_MAX;
    else
        return BG_MODE_FILL;
}

static void get_xdg_pictures_dir(Config *config) {
    const gchar *xdg_pictures_dir =
        g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    if (!xdg_pictures_dir) {
        xdg_pictures_dir = "";
    }
    config->source_directory = strdup(xdg_pictures_dir);
}

extern void update_source_directory(Config *config, const gchar *new_src_dir) {
    free(config->source_directory);
    gushort src_dir_len = strlen(new_src_dir) + 1;
    config->source_directory = malloc(src_dir_len);
    snprintf(config->source_directory, src_dir_len, "%s", new_src_dir);
}

extern Config *load_config() {
    gchar *config_filename = get_config_file();
    FILE *file = fopen(config_filename, "r");
    free(config_filename);
    Config *config = (Config *)malloc(sizeof(Config));

    if (!config) {
        perror("Memory allocation failed");
        return NULL;
    }

    config->monitors_with_backgrounds = NULL;
    config->number_of_monitors = 0;

    if (file == NULL) {
        get_xdg_pictures_dir(config);
        return config;
    }

    size_t capacity = 255;
    gchar *file_content = malloc(capacity);
    if (!file_content) {
        perror("Memory allocation failed");
        free(config);
        fclose(file);
        return NULL;
    }
    file_content[0] = '\0';

    size_t file_size = 0;
    gchar line[100];

    while (fgets(line, sizeof(line), file)) {
        size_t line_len = strlen(line);
        if (file_size + line_len + 1 > capacity) {
            capacity *= 2;
            gchar *temp = realloc(file_content, capacity);
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
    } else {
        get_xdg_pictures_dir(config);
    }

    cJSON *monitors_json = cJSON_GetObjectItemCaseSensitive(
        settings_json, "monitorsWithBackgrounds");
    cJSON *monitor_background_json;
    int number_of_monitors = 0;

    cJSON_ArrayForEach(monitor_background_json, monitors_json) {
        ConfigMonitor *temp =
            realloc(config->monitors_with_backgrounds,
                    (number_of_monitors + 1) * sizeof(ConfigMonitor));

        if (!temp) {
            perror("Memory reallocation failed");
            cJSON_Delete(settings_json);
            free_config(config);
            return NULL;
        }
        config->monitors_with_backgrounds = temp;

        ConfigMonitor *monitor_background_pair =
            &config->monitors_with_backgrounds[number_of_monitors];
        memset(monitor_background_pair, 0, sizeof(ConfigMonitor));

        cJSON *monitor_name_json =
            cJSON_GetObjectItemCaseSensitive(monitor_background_json, "name");
        cJSON *background_json = cJSON_GetObjectItemCaseSensitive(
            monitor_background_json, "imagePath");
        cJSON *bg_mode_json =
            cJSON_GetObjectItemCaseSensitive(monitor_background_json, "bgMode");

        cJSON *bg_fallback_color_json = cJSON_GetObjectItemCaseSensitive(
            monitor_background_json, "bgFallbackColor");

        if (!cJSON_IsString(monitor_name_json) ||
            !monitor_name_json->valuestring) {
            fprintf(stderr, "Warning: Monitor name is missing or invalid, "
                            "skipping this monitor.\n");
            continue;
        }
        if (!cJSON_IsString(background_json) || !background_json->valuestring) {
            fprintf(stderr, "Warning: Image path is missing or invalid, "
                            "skipping this monitor.\n");
            continue;
        }

        monitor_background_pair->name = strdup(monitor_name_json->valuestring);
        if (!monitor_background_pair->name) {
            perror("Memory allocation failed for monitor name");
            cJSON_Delete(settings_json);
            free_config(config);
            return NULL;
        }

        monitor_background_pair->image_path =
            strdup(background_json->valuestring);
        if (!monitor_background_pair->image_path) {
            perror("Memory allocation failed for image path");
            free(monitor_background_pair->name);
            cJSON_Delete(settings_json);
            free_config(config);
            return NULL;
        }

        if (cJSON_IsString(bg_mode_json)) {
            monitor_background_pair->bg_mode =
                bg_mode_from_string(bg_mode_json->valuestring);
        } else {
            monitor_background_pair->bg_mode = BG_MODE_FILL;
        }

        if (cJSON_IsString(bg_fallback_color_json)) {
            monitor_background_pair->bg_fallback_color =
                g_strdup(bg_fallback_color_json->valuestring);
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
    gchar *string = NULL;
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
        ConfigMonitor *monitor_background_pair =
            &config->monitors_with_backgrounds[i];

        if (monitor_background_pair->bg_fallback_color) {
            cJSON_AddStringToObject(monitor_background_json,
                                    "bgFalllbackColor",
                                    monitor_background_pair->bg_fallback_color);
        }

                monitor_background_json, "bgMode",
                bg_mode_to_string(monitor_background_pair->bg_mode)) == NULL) {
            goto end;
        }

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
