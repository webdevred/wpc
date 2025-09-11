// Copyright 2025 webdevred

#include <glib.h>

#include "wpc/rendering_region.h"
#include "wpc/wallpaper_transformation.h"
#include "wpc/wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

/*
 * Function: transform_wallpaper
 * -----------------------------
 * Transforms a wallpaper image based on the selected background mode and
 * monitor properties. It resizes, crops, and composites the image onto a new
 * canvas to match the display configuration.
 *
 * Parameters:
 *   - wand_ptr: Pointer to the MagickWand containing the image. The pointer
 * will be updated to reference the newly transformed image.
 *   - monitor: Pointer to the Monitor struct containing screen dimensions.
 *   - bg_mode: Specifies how the image should be rendered
 *   - bg_fallback_color: Fallback background color if none is specified.
 *
 * Returns:
 *   Void. The original image is replaced with the transformed image.
 *
 * Notes:
 *   - This function creates a new MagickWand and frees the old one.
 */

extern void transform_wallpaper(MagickWand **wand_ptr, Monitor *monitor,
                                BgMode bg_mode,
                                const gchar *bg_fallback_color) {
    RenderingRegion rr;
    MagickWand *wand, *scaled_wand;
    PixelWand *color;
    wand = *wand_ptr;

    rr = create_rendering_region(wand, monitor, bg_mode);

    color = NewPixelWand();
    PixelSetColor(color, bg_fallback_color);

    scaled_wand = NewMagickWand();
    MagickNewImage(scaled_wand, monitor->width, monitor->height, color);
#ifdef WPC_IMAGEMAGICK_7
    MagickResizeImage(wand, rr.width, rr.height, LanczosFilter);
    MagickCropImage(wand, rr.width, rr.height, rr.src_x, rr.src_y);
    MagickCompositeImage(scaled_wand, wand, OverCompositeOp, MagickTrue,
                         rr.monitor_x, rr.monitor_y);
#else
    MagickResizeImage(wand, rr.width, rr.height, LanczosFilter, 1.0);
    MagickCropImage(wand, rr.width, rr.height, rr.src_x, rr.src_y);

    MagickCompositeImage(scaled_wand, wand, OverCompositeOp, rr.monitor_x,
                         rr.monitor_y);
#endif

    DestroyPixelWand(color);
    wand = NULL;
    DestroyMagickWand(*wand_ptr);

    *wand_ptr = scaled_wand;
}

/*
 * Function: transform_wallpaper_tiled
 * -----------------------------------
 * Generates a tiled version of the wallpaper by repeating the original image
 * to fully cover the monitor dimensions.
 *
 * Parameters:
 *   - wand_ptr: Pointer to the MagickWand containing the image. The pointer
 * will be updated to reference the newly tiled image.
 *   - monitor: Pointer to the Monitor struct containing screen dimensions.
 *
 * Returns:
 *   - true: If the image was successfully tiled and updated.
 *   - false: If the image is too large to be tiled properly.
 *
 * Notes:
 *   - This function creates a new MagickWand and frees the old one.
 *   - The function assumes the original image is smaller than the monitor.
 */
extern bool transform_wallpaper_tiled(MagickWand **wand_ptr, Monitor *monitor) {
    glong amount_x, amount_y;
    gulong output_width, output_height, img_w, img_h;
    MagickWand *wand, *tiled_wand;
    PixelWand *color;
    guint i, j;
    wand = *wand_ptr;
    img_w = MagickGetImageWidth(wand);
    img_h = MagickGetImageHeight(wand);

    if (img_w > monitor->width || img_h > monitor->height) return FALSE;

    amount_x = (monitor->width + (glong)img_w - 1) / (glong)img_w;
    amount_y = (monitor->height + (glong)img_h - 1) / (glong)img_h;

    output_width = (gulong)amount_x * img_w;
    output_height = (gulong)amount_y * img_h;

    color = NewPixelWand();
    PixelSetColor(color, "none");

    tiled_wand = NewMagickWand();
    MagickNewImage(tiled_wand, output_width, output_height, color);
    for (i = 0; i < amount_x; i++) {
        for (j = 0; j < amount_y; j++) {
#ifdef WPC_IMAGEMAGICK_7
            MagickCompositeImage(tiled_wand, wand, OverCompositeOp, MagickTrue,
                                 (glong)img_w * i, (glong)img_h * j);
#else
            MagickCompositeImage(tiled_wand, wand, OverCompositeOp, img_w * i,
                                 img_h * j);
#endif
        }
    }
    MagickCropImage(wand, monitor->width, monitor->height, 0, 0);
    DestroyPixelWand(color);
    wand = NULL;
    DestroyMagickWand(*wand_ptr);

    *wand_ptr = tiled_wand;
    return true;
}
