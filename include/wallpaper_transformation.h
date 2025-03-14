#include <wand/MagickWand.h>
#include "rendering_region.h"

extern bool transform_wallpaper_tiled(MagickWand **wand_ptr, Monitor *monitor);

extern void transform_wallpaper(MagickWand **wand_ptr, Monitor* monitor, BgMode bg_mode, const gchar* conf_bg_fb_color);
