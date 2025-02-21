#pragma once
#include "wpc.h"
#include <glib.h>
typedef struct {
    gushort width;
    gushort height;
    gushort vertical_position;
    gushort horizontal_position;
    bool primary;
    gchar *name;
} Monitor;

typedef struct {
    gushort width;
    gushort height;
    gchar *path;
} Wallpaper;

typedef struct {
    void *data;
    gushort amount_allocated;
    gushort amount_used;
} ArrayWrapper;

