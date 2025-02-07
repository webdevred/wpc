#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wpc.h"
#include "common.h"

extern int parse_config(char ***config_ptr, int *lines_ptr) {
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
