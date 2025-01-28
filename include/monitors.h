#pragma once

#include <stdbool.h>

typedef struct {
    int width;
    int height;
    bool primary;
} Monitor;

extern void wm_list_monitors(Monitor **monitors, int *number_of_monitors);

extern void dm_list_monitors(Monitor **primary_monitor,
                             Monitor **other_monitors,
                             int *number_of_other_monitors);
