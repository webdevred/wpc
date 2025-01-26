#include "monitors.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define MAX_LINE_LENGTH 1024

static int parse_config(char ***config_ptr, int *lines_ptr) {
    FILE *file = fopen("/etc/lightdm/slick-greeter.conf", "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    int lines = 0;
    char **config = NULL;

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        config = realloc(config, (lines + 1) * sizeof(char *));
        if (config == NULL) {
            perror("Error reallocating memory");
            fclose(file);
            return -1;
        }

        config[lines] = malloc((strlen(line) + 1) * sizeof(char));
        if (config[lines] == NULL) {
            perror("Error allocating memory for line");
            fclose(file);
            return -1;
        }
        config[lines] = strdup(line);

        lines++;
    }

    fclose(file);

    *config_ptr = config;
    *lines_ptr = lines;
    return 0;
}

extern void lightdm_get_backgrounds(char **primary_monitor,
                                    char **other_monitor) {
    char **config;
    int expected_lines;
    if (parse_config(&config, &expected_lines) == 0) {
        for (int i = 0; i < expected_lines; i++) {
            char **line = &config[i];
            char *delimeter = strchr(*line, '=');
            if (delimeter) {
                *delimeter = '\0';
                if (strcmp(*line, "background")) {
                    *primary_monitor =
                        malloc((strlen(*line) + 1) * sizeof(char));
                    *primary_monitor = strdup(delimeter + 1);
                } else if (strcmp(*line, "other-monitors-logo")) {
                    *other_monitor = malloc((strlen(*line) + 1) * sizeof(char));
                    *other_monitor = strdup(delimeter + 1);
                }
            }
            free(config[i]);
        }
        free(config);
    }
}

extern void lightdm_set_background(gchar *image_path, Monitor *monitor) {
    char **config;
    int expected_lines;
    if (parse_config(&config, &expected_lines) == 0) {
        for (int i = 0; i < expected_lines; i++) {
            char **line = &config[i];
            char *delimeter = strchr(*line, '=');
            bool line_modified = false;
            char *line_to_write;
            if (delimeter) {
                *delimeter = '\0';
                if (strcmp(*line, "background")) {
                    int length = strlen(*line) + strlen(image_path) + 2;
                    line_to_write = malloc(length * sizeof(char));
                    snprintf(line_to_write, length, "%s=%s", *line, image_path);
                } else if (strcmp(*line, "other-monitors-logo")) {
                    int length = strlen(*line) + strlen(image_path) + 2;
                    line_to_write = malloc(length * sizeof(char));
                    snprintf(line_to_write, length, "%s=%s", *line, image_path);
                }
            }

            if (!line_modified) {
                *delimeter = '=';
                line_to_write = malloc((strlen(*line) + 1) * sizeof(char));
                line_to_write = strdup(*line);
            }
            free(config[i]);
        }
    }
    free(config);
}

/* labb */

/* #include <gtk/gtk.h> */
/* #include <stdio.h> */
/* #include <stdlib.h> */

/* #include "monitors.h" */
/* #define MAX_LINE_LENGTH 256 */

/* typedef struct { */
/*     char *key; */
/*     char *value; */
/* } ConfigItem; */

/* typedef struct { */
/*     char *key; */
/*     ConfigItem *items; */
/* } ConfigSection; */

/* static int parse_config(ConfigSection **config) { */
/*     FILE *file = fopen("/etc/lightdm/slick-greeter.conf", "r"); */
/*     if (file == NULL) { */
/*         perror("Error opening file"); */
/*         return -1; */
/*     } */

/*     int number_of_sections = 0; */
/*     int number_of_section_items = 0; */

/*     char line[MAX_LINE_LENGTH]; */
/*     while (fgets(line, sizeof(line), file)) { */
/*         if (line[0] == '\0') continue; */

/*         if (line[0] == '[' && line[strlen(line) - 2] == ']') { */
/*             number_of_section_items = 0; */
/*             ConfigSection *config = realloc(config, (number_of_sections +
 * 1)
 * * */
/*                                                         sizeof(ConfigSection));
 */
/*             ConfigSection *section = &config[number_of_sections]; */

/*             line[strlen(line) - 2] = '\0'; */
/*             char* section_key; */
/*             strncpy(section_key, line + 1, sizeof(section->key) - 1); */
/*             section->key = malloc((strlen(section_key) + 1) *
 * sizeof(char= */
/*             strcpy(section->key, section_key); */
/*             continue; */
/*         } */

/*         char *delimiter = strchr(line, '='); */
/*         if (delimiter) { */
/*             *delimiter = '\0'; */

/*             config[number_of_sections]->items = */
/*                 realloc(config[number_of_sections]->items, */
/*                         (number_of_section_items + 1) *
 * sizeof(ConfigItem));
 */

/*             ConfigItem *item = */
/*                 &config[number_of_sections]->items[number_of_section_items];
 */

/*             item->key = malloc(sizeof(char) * (strlen(line) + 1)); */
/*             item->key = malloc(sizeof(char) * (strlen(delimiter + 1) +
 * 1));
 */

/*             strncpy(item->key, line, MAX_LINE_LENGTH); */
/*             strncpy(item->value, delimiter + 1, MAX_LINE_LENGTH); */
/*         } */
/*     } */
/* } */

/* extern void lightdm_set_background_primary(gchar *image_path, */
/*                                            Monitor *monitor) { */
/*   ConfigSection* config; */
/*   parse_config(&config); */
/*   while (true) { */
/*     ConfigSection section = *config; */
/*     printf("%s", section.key); */
/*   } */
/* } */
