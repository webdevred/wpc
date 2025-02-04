#pragma once

#include "wallpaper_struct.h"

extern Wallpaper *list_wallpapers(char *source_directory,
                                  int *number_of_images);

extern Wallpaper get_wallpaper(char *wallpaper_path);
