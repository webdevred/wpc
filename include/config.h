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

void free_config(Config *config);

Config *load_config(void);

void dump_config(Config *config);
