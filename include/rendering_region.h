#pragma once

#include "config.h"
#include "monitors.h"
#include "wpc_imagemagick.h"
#include <glib.h>

typedef struct {
    gushort width, height;
    gushort src_x, src_y;
    gshort monitor_x, monitor_y;
} RenderingRegion;

extern RenderingRegion *
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode);
