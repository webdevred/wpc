#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define MAX_LINE_LENGTH 1024

extern int lightdm_parse_config(char ***config_ptr, int *lines_ptr) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Error opening configuration file");
        return -1;
    }

    int lines = 0;
    char **config = *config_ptr;
    char line[MAX_LINE_LENGTH];

    if (config) {
        for (int i = 0; i < *lines_ptr; i++) {
            free(config[i]);
        }
        free(config);
        config = NULL;
    }

    while (fgets(line, sizeof(line), file)) {
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
