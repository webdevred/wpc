#pragma once

#include "config.h"
#include "filesystem.h"
#include "monitors.h"

extern void set_wallpapers(Config *config, WallpaperQueue *queue,
                           MonitorArray *mon_arr_wrapper);
extern void init_x(void);
