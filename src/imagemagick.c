#include "structs.h"
#include <wand/MagickWand.h>

int set_resolution(Wallpaper *wallpaper) {
    MagickWandGenesis();

    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper->path) == MagickFalse) {
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
