#pragma once

#include "wpc.h"

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int vertical_position;
    unsigned int horizontal_position;
    bool primary;
    char name[32];
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
