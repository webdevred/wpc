#pragma once

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int vertical_position;
    unsigned int horizontal_position;
    bool primary;
    char *name;
} Monitor;

typedef struct {
    unsigned int width;
    unsigned int height;
    char *path;
} Wallpaper;
