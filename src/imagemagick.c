#include "structs.h"
#include <wand/MagickWand.h>

extern int set_resolution(const char *wallpaper_path, Wallpaper *wallpaper) {
    MagickWandGenesis();

    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper_path) == MagickFalse) {
        fprintf(stderr, "Failed to read image: %s\n", wallpaper->path);
        return 1;
    }

    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);

    wallpaper->width = width;
    wallpaper->height = height;

    DestroyMagickWand(wand);
    MagickWandTerminus();
    return 0;
}
