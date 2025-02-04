#pragma once
#include <stdbool.h>

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int vertical_position;
    unsigned int horizontal_position;
    bool primary;
    char *name;
} Monitor;

extern Monitor *wm_list_monitors(int *number_of_monitors);

extern void dm_list_monitors(Monitor *primary_monitor,
                             Monitor *secondary_monitor,
                             int *number_of_other_monitors);

extern Monitor *get_monitor(char *monitor_name);
