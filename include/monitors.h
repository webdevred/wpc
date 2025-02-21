#pragma once
#include "structs.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>

extern void init_disp(Display **disp, Window *root);

extern void free_monitors(ArrayWrapper *arr);

extern ArrayWrapper *list_monitors(const bool virtual_monitors);

extern Monitor *get_monitor(const gchar *monitor_name);
