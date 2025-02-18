#pragma once

#include "wpc.h"

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int vertical_position;
    unsigned int horizontal_position;
    bool primary;
    char *name;
} Monitor;

typedef struct {
    ushort width;
    ushort height;
    char *path;
} Wallpaper;

typedef struct {
    void *data;
    unsigned int amount_allocated;
    unsigned int amount_used;
} ArrayWrapper;
