#pragma once

#include <glib.h>
#include "structs.h"

extern void init_bgpair(Config **config, const gchar* monitor_name);

extern void free_config(Config *config);

extern void update_source_directory(Config *config, const gchar *new_src_dir);

extern Config *load_config(void);

extern void dump_config(Config *config);
