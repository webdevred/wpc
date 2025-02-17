#pragma once
#include "structs.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>

extern void init_disp(Display **disp, Window *root);

extern void get_screen_resources(Display **disp, Window *root,
                                 XRRScreenResources **screen_resources);

extern void free_monitors(ArrayWrapper *arr);

extern ArrayWrapper *list_monitors();

extern Monitor *get_monitor(char *monitor_name);
