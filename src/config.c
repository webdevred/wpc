// Copyright 2025 webdevred

#include <cjson/cJSON.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wpc/common.h"
#include "wpc/config.h"

#define CONFIG_FILE ".config/wpc/settings.json"

static gboolean is_empty_string(const gchar *string_ptr) {
    return string_ptr == NULL || g_strcmp0(string_ptr, "") == 0;
}

static gboolean validate_src_dir(const gchar *source_directory) {
    GDir *dir;
    gboolean valid;
    if (is_empty_string(source_directory)) return FALSE;
    dir = g_dir_open(source_directory, 0, NULL);
    valid = FALSE;
    if (dir) {
        valid = TRUE;
        g_dir_close(dir);
    }

    return valid;
}

static gchar *get_config_file(void) {
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
    gulong old_color_len;
    gchar new_color[8];
    gulong new_color_index = 1;
    gulong old_color_index = 0;
    gchar *old_color = config->bg_fallback_color;
    if (is_empty_string(old_color)) goto invalidate;

    old_color_len = strlen(old_color);
    new_color[0] = '#';

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
    guint amount;
    ConfigMonitor *monitors, *new_monitor, *new_monitors;
    monitors = config->monitors_with_backgrounds;
    amount = config->number_of_monitors;

    new_monitors = realloc(monitors, (amount + 1) * sizeof(ConfigMonitor));
    if (!new_monitors) {
        g_warning("Error: Memory allocation failed.\n");
        return;
    }

    config->monitors_with_backgrounds = new_monitors;
    new_monitor = &new_monitors[amount];

    new_monitor->name = g_strdup(monitor_name);
    new_monitor->image_path = g_strdup(wallpaper_path);
    new_monitor->bg_mode = bg_mode;
    new_monitor->bg_fallback_color = g_strdup("");
    new_monitor->valid_bg_fallback_color = g_strdup("");
}

extern Config *load_config(void) {
    gchar *temp;
    size_t capacity, file_size;
    gchar *file_content;
    gchar line[100];
    gchar *config_filename = get_config_file();
    ConfigMonitor *temp_config_monitor;
    gushort number_of_monitors = 0;
    Config *config;
    ConfigMonitor *monitor_background_pair;
    cJSON *settings_json, *monitor_name_json, *monitors_json,
        *monitor_background_json, *bg_mode_json, *bg_fallback_color_json,
        *source_directory_json;
    FILE *file;
    file = fopen(config_filename, "r");
    free(config_filename);
    config = (Config *)malloc(sizeof(Config));
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

    capacity = 255;
    file_content = malloc(capacity);
    if (!file_content) {
        perror("Memory allocation failed");
        free(config);
        fclose(file);
        return NULL;
    }
    file_content[0] = '\0';

    file_size = 0;
    while (fgets(line, sizeof(line), file)) {
        size_t line_len = strlen(line);

        if (file_size + line_len + 1 > capacity) {
            capacity *= 2;
            temp = realloc(file_content, capacity);
            if (!temp) {
                perror("Memory reallocation failed");
                free(file_content);
                free_config(config);
                fclose(file);
                return NULL;
            }
            file_content = temp;
        }

        memcpy(file_content + file_size, line, line_len + 1);
        file_size += line_len;
    }

    fclose(file);

    settings_json = cJSON_Parse(file_content);
    free(file_content);

    if (!settings_json) {
        fprintf(stderr, "JSON parsing error\n");
        free_config(config);
        return NULL;
    }

    source_directory_json =
        cJSON_GetObjectItemCaseSensitive(settings_json, "sourceDirectoryPath");
    if (cJSON_IsString(source_directory_json) &&
        source_directory_json->valuestring) {
        config->source_directory = g_strdup(source_directory_json->valuestring);
    } else {
        get_xdg_pictures_dir(config);
    }

    config->valid_source_directory =
        validate_src_dir(config->source_directory) ? TRUE : FALSE;

    monitors_json = cJSON_GetObjectItemCaseSensitive(settings_json,
                                                     "monitorsWithBackgrounds");

    cJSON_ArrayForEach(monitor_background_json, monitors_json) {
        temp_config_monitor =
            realloc(config->monitors_with_backgrounds,
                    (number_of_monitors + 1) * sizeof(ConfigMonitor));

        if (!temp_config_monitor) {
            perror("Memory reallocation failed");
            cJSON_Delete(settings_json);
            free_config(config);
            return NULL;
        }
        config->monitors_with_backgrounds = temp_config_monitor;

        monitor_background_pair =
            &config->monitors_with_backgrounds[number_of_monitors];
        memset(monitor_background_pair, 0, sizeof(ConfigMonitor));

        monitor_name_json =
            cJSON_GetObjectItemCaseSensitive(monitor_background_json, "name");
        monitor_background_json = cJSON_GetObjectItemCaseSensitive(
            monitor_background_json, "imagePath");
        bg_mode_json =
            cJSON_GetObjectItemCaseSensitive(monitor_background_json, "bgMode");

        bg_fallback_color_json = cJSON_GetObjectItemCaseSensitive(
            monitor_background_json, "bgFallbackColor");

        if (!cJSON_IsString(monitor_name_json) ||
            !monitor_name_json->valuestring) {
            fprintf(stderr, "Warning: Monitor name is missing or invalid, "
                            "skipping this monitor.\n");
            continue;
        }
        if (!cJSON_IsString(monitor_background_json) ||
            !monitor_background_json->valuestring) {
            fprintf(stderr, "Warning: Image path is missing or invalid, "
                            "skipping this monitor.\n");
            continue;
        }

        monitor_background_pair->name =
            g_strdup(monitor_name_json->valuestring);
        if (!monitor_background_pair->name) {
            perror("Memory allocation failed for monitor name");
            cJSON_Delete(settings_json);
            free_config(config);
            return NULL;
        }

        monitor_background_pair->image_path =
            g_strdup(monitor_background_json->valuestring);
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
    ConfigMonitor *monitor_background_pair;
    cJSON *settings_json, *monitors_with_backgrounds_json;
    gchar *filename;
    FILE *file;
    char *string;
    BgMode bg_mode;
    gushort number_of_monitors;
    string = malloc(sizeof(char) * 3);
    string = "{}";
    filename = get_config_file();
    create_parent_dirs(filename, 0770);
    file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening configuration file");
        exit(1);
    }
    monitors_with_backgrounds_json = NULL;

    settings_json = cJSON_CreateObject();

    if (cJSON_AddStringToObject(settings_json, "sourceDirectoryPath",
                                config->source_directory) == NULL) {
        goto end;
    }

    monitors_with_backgrounds_json =
        cJSON_AddArrayToObject(settings_json, "monitorsWithBackgrounds");
    if (monitors_with_backgrounds_json == NULL) {
        goto end;
    }

    number_of_monitors = config->number_of_monitors;
    for (int i = 0; i < number_of_monitors; i++) {
        cJSON *monitor_background_json = cJSON_CreateObject();
        monitor_background_pair = &config->monitors_with_backgrounds[i];

        if (!is_empty_string(monitor_background_pair->bg_fallback_color)) {
            if (cJSON_AddStringToObject(
                    monitor_background_json, "bgFallbackColor",
                    monitor_background_pair->bg_fallback_color) == NULL) {
                cJSON_Delete(monitor_background_json);
                goto end;
            }
        }

        bg_mode = monitor_background_pair->bg_mode;

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

        cJSON_AddItemToArray(monitors_with_backgrounds_json,
                             monitor_background_json);
    }

    string = cJSON_Print(settings_json);
    if (string == NULL) {
        fprintf(stderr, "Failed to print monitor.\n");
    }
    fwrite(string, sizeof(char), strlen(string), file);
    fflush(file);
end:
    cJSON_Delete(settings_json);
    fclose(file);
    free(string);
}
