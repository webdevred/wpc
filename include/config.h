#pragma once

#include "monitors.h"
#include <glib.h>

typedef struct {
    char *name;
    char *image_path;
} MonitorBackgroundPair;

typedef struct {
    int number_of_monitors;
    MonitorBackgroundPair *monitors_with_backgrounds;
    char *source_directory;
} Config;

extern int lightdm_parse_config(char ***config_ptr, int *lines_ptr);

extern void free_config(Config *config);

extern void update_source_directory(Config *config, const char *new_src_dir);

extern Config *load_config(void);

extern void dump_config(Config *config);
