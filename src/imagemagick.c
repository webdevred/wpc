#include <stdio.h>
#include <stdlib.h>
#include <wand/MagickWand.h>

#include "monitors.h"
#include "resolution_scaling.h"
#include "wallpaper_struct.h"

int scale_image(Wallpaper *src_image, char *dst_image_path, Monitor *monitor) {
#define ThrowWandException(wand)                                               \
    {                                                                          \
        char *description;                                                     \
                                                                               \
        ExceptionType severity;                                                \
                                                                               \
        description = MagickGetException(wand, &severity);                     \
        (void)fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(),             \
                      description);                                            \
        description = (char *)MagickRelinquishMemory(description);             \
        exit(-1);                                                              \
    }

    MagickBooleanType status;

    MagickWand *magick_wand;

    MagickWandGenesis();
    magick_wand = NewMagickWand();
    status = MagickReadImage(magick_wand, src_image->path);
    if (status == MagickFalse) ThrowWandException(magick_wand);

    int aspect_image_height =
        scale_height(src_image->width, src_image->height, monitor->width);
    int vertical_offset = (int)((aspect_image_height - monitor->height) / 2);

    if (vertical_offset < 0) vertical_offset = 0;

    MagickResetIterator(magick_wand);
    if (MagickNextImage(magick_wand) != MagickFalse) {
        MagickResizeImage(magick_wand, monitor->width, aspect_image_height,
                          LanczosFilter, 1.0);
        MagickCropImage(magick_wand, monitor->width, monitor->height, 0,
                        vertical_offset);
    }

    status = MagickWriteImages(magick_wand, dst_image_path, MagickTrue);
    if (status == MagickFalse) ThrowWandException(magick_wand);
    magick_wand = DestroyMagickWand(magick_wand);
    MagickWandTerminus();
    return (0);
}
