#include <glib.h>
#include <wand/MagickWand.h>
#include "structs.h"
#include "config.h"

typedef struct {
    gushort width;
    gushort height;
    gushort src_x;
    gushort src_y;
    gushort monitor_x;
    gushort monitor_y;
} RenderingRegion;

extern RenderingRegion *create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode);
