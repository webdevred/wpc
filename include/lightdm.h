#pragma once
#include "monitors.h"

extern int parse_config(char ***config_ptr, int *lines_ptr);

extern void lightdm_set_background(Wallpaper *wallpaper, Monitor *monitor);
