#pragma once
#include <stdbool.h>

typedef struct {
    int width;
    int height;
    bool primary;
} Monitor;

extern void list_monitors(Monitor **monitors, int *number_of_monitors);
