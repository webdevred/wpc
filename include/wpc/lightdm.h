#pragma once
#include "wpc/config.h"
#include "wpc/monitors.h"

extern void lightdm_set_background(Wallpaper *wallpaper, Monitor *monitor,
                                   BgMode bg_mode);
