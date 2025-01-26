#include <stdbool.h>

typedef struct {
    int width;
    int height;
    bool primary;
} Monitor;

#pragma once

extern void wm_list_monitors(Monitor **monitors, int *number_of_monitors);

extern void dm_list_monitors(Monitor **primary_monitor,
                             int *number_of_other_monitors);
