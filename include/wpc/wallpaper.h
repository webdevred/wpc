#pragma once

#include "wpc/config.h"
#include "wpc/filesystem.h"
#include "wpc/monitors.h"

extern void set_wallpapers(Config *config, WallpaperQueue *queue,
                           MonitorArray *mon_arr_wrapper);
extern void init_x(void);
