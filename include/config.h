#pragma once

#include "monitors.h"
#include <glib.h>

typedef struct {
    char *name;
    char *image_path;
} MonitorBackgroundPair;

typedef struct {
    char *source_directory;
    int number_of_monitors;
    MonitorBackgroundPair *monitors_with_backgrounds;
} Config;

extern int lightdm_parse_config(char ***config_ptr, int *lines_ptr);

extern void free_config(Config *config);

extern Config *load_config(void);

extern void dump_config(Config *config);
