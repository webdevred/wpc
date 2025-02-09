#pragma once
#include "structs.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>

extern void init_disp(Display **disp, Window *root);

extern void get_screen_resources(Display **disp, Window *root,
                                 XRRScreenResources **screen_resources);

extern Monitor *wm_list_monitors(int *number_of_monitors);

extern void dm_list_monitors(Monitor *primary_monitor,
                             Monitor *secondary_monitor,
                             int *number_of_other_monitors);

extern Monitor *get_monitor(char *monitor_name);
