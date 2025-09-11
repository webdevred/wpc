#pragma once

#include "config.h"
#include "wpc/monitors.h"
#include <glib.h>

typedef struct _MagickWand MagickWand;

typedef struct {
    gulong width, height;
    glong src_x, src_y;
    glong monitor_x, monitor_y;
} RenderingRegion;

extern RenderingRegion
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode);
