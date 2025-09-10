#pragma once

#include "config.h"
#include "wpc/monitors.h"
#include <glib.h>

typedef struct _MagickWand MagickWand;

typedef struct {
    gushort width, height;
    gushort src_x, src_y;
    gshort monitor_x, monitor_y;
} RenderingRegion;

extern RenderingRegion
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode);
