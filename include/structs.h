#pragma once

#include "wpc.h"
typedef struct {
    ushort width;
    ushort height;
    ushort vertical_position;
    ushort horizontal_position;
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
    ushort amount_allocated;
    ushort amount_used;
} ArrayWrapper;
