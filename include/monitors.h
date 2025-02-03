#pragma once
#include <stdbool.h>

typedef struct {
    unsigned int width;
    unsigned int height;
    bool primary;
    char *name;
} Monitor;

extern Monitor *wm_list_monitors(int *number_of_monitors);

extern void dm_list_monitors(Monitor *primary_monitor,
                             Monitor *secondary_monitor,
                             int *number_of_other_monitors);
