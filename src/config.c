#include <cjson/cJSON.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define CONFIG_FILE ".config/wpc/settings.json"

static gboolean is_empty_string(const gchar *string_ptr) {
    return string_ptr == NULL || g_strcmp0(string_ptr, "") == 0;
}

static gboolean validate_src_dir(const gchar *source_directory) {
    if (is_empty_string(source_directory)) return FALSE;
    GDir *dir = g_dir_open(source_directory, 0, NULL);
    gboolean valid = FALSE;
    if (dir) {
        valid = TRUE;
        g_dir_close(dir);
    }

    return valid;
}

static gchar *get_config_file() {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s/%s", home, CONFIG_FILE);
}

static void free_config_monitor(ConfigMonitor *pair) {
    if (!pair) return;

    if (pair->bg_fallback_color) {
        free(pair->bg_fallback_color);
        pair->bg_fallback_color = NULL;
    }

    if (pair->valid_bg_fallback_color) {
        free(pair->valid_bg_fallback_color);
        pair->valid_bg_fallback_color = NULL;
    }

    if (pair->name) {
        free(pair->name);
        pair->name = NULL;
    }
    if (pair->image_path) {
        free(pair->image_path);
        pair->image_path = NULL;
    }
}

static const gchar *bg_mode_to_string(BgMode type) {
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

static BgMode bg_mode_from_string(const gchar *str) {
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
        xdg_pictures_dir = g_strdup("");
    }
    config->source_directory = g_strdup(xdg_pictures_dir);
}

extern void free_config(Config *config) {
    if (!config) return;

    free(config->source_directory);
    if (config->monitors_with_backgrounds) {
        for (int i = 0; i < config->number_of_monitors; i++) {
            free_config_monitor(&config->monitors_with_backgrounds[i]);
        }
        free(config->monitors_with_backgrounds);
        config->monitors_with_backgrounds = NULL;
    }

    free(config);
    config = NULL;
}

extern void update_source_directory(Config *config, const gchar *new_src_dir) {
    if (validate_src_dir(new_src_dir)) {
        free(config->source_directory);
        config->source_directory = g_strdup(new_src_dir);
        config->valid_source_directory = TRUE;
    }
}

static gboolean validate_bg_fallback(ConfigMonitor *config) {
    gchar *old_color = config->bg_fallback_color;
    if (is_empty_string(old_color)) goto invalidate;

    guint old_color_len = strlen(old_color);
    gchar new_color[8];
    new_color[0] = '#';
    guint new_color_index = 1;
    guint old_color_index = 0;

    if (old_color[0] == '#') {
        if (old_color_len == 4 || old_color_len == 7) {
            old_color_index = 1;
        } else {
            goto warn;
        }
    } else {
        if (old_color_len == 3 || old_color_len == 6) {
            old_color_index = 0;
        } else {
            goto warn;
        }
    }

    for (; old_color_index < old_color_len; old_color_index++) {
        gchar color_cell = old_color[old_color_index];
        if (color_cell < '0' || color_cell > 'f') goto warn;
        new_color[new_color_index++] = color_cell;
        if (old_color_len == 3) {
            new_color[new_color_index++] = color_cell;
        }
    }

    new_color[new_color_index] = '\0';

    config->valid_bg_fallback_color = g_strdup(new_color);
    return TRUE;

warn:
    g_warning("invalid fallback_bg: %s", old_color);

invalidate:
    config->valid_bg_fallback_color = g_strdup("#ff0000");
    return FALSE;
}

extern void init_config_monitor(Config *config, const gchar *monitor_name,
                                const gchar *wallpaper_path,
                                const BgMode bg_mode) {
    ConfigMonitor *monitors = config->monitors_with_backgrounds;
    guint amount = config->number_of_monitors;

    ConfigMonitor *new_monitors =
        realloc(monitors, (amount + 1) * sizeof(ConfigMonitor));
    if (!new_monitors) {
        g_warning("Error: Memory allocation failed.\n");
        return;
    }

    config->monitors_with_backgrounds = new_monitors;
    ConfigMonitor *new_monitor = &new_monitors[amount];

    new_monitor->name = g_strdup(monitor_name);
    new_monitor->image_path = g_strdup(wallpaper_path);
    new_monitor->bg_mode = bg_mode;
    new_monitor->bg_fallback_color = g_strdup("");
    new_monitor->valid_bg_fallback_color = g_strdup("");
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
        config->valid_source_directory =
            validate_src_dir(config->source_directory) ? TRUE : FALSE;
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

    config->valid_source_directory =
        validate_src_dir(config->source_directory) ? TRUE : FALSE;

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
        } else {
            monitor_background_pair->bg_fallback_color = g_strdup("");
        }

        validate_bg_fallback(monitor_background_pair);

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

        if (!is_empty_string(monitor_background_pair->bg_fallback_color)) {
            if (cJSON_AddStringToObject(
                    monitor_background_json, "bgFallbackColor",
                    monitor_background_pair->bg_fallback_color) == NULL) {
                cJSON_Delete(monitor_background_json);
                goto end;
            }
        }

        BgMode bg_mode = monitor_background_pair->bg_mode;

        if (bg_mode) {
            cJSON_AddStringToObject(monitor_background_json, "bgMode",
                                    bg_mode_to_string(bg_mode));
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
    fwrite(string, sizeof(char), strlen(string), file);
    fflush(file);
end:
    cJSON_Delete(settings);
    fclose(file);
    free(string);
}
