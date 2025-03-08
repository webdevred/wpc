#include "rendering_region.h"

/*
 * Function: create_rendering_region
 * ---------------------------------
 * Computes and returns a RenderingRegion based on the given image (wand),
 * monitor configuration, and background mode. The rendering region determines
 * how the image is displayed on the monitor.
 *
 * Parameters:
 *   - wand: Pointer to the MagickWand containing the image.
 *   - monitor: Pointer to the Monitor struct with monitor dimensions and
 * position.
 *   - bg_mode: Specifies how the image should be rendered. Please BG_MODES.org
 * for explanation for information.
 *
 * Returns:
 *   A pointer to the allocated RenderingRegion with computed source offsets and
 * dimensions.
 */
extern RenderingRegion *
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode) {
    gushort img_w, img_h;

    gushort mon_w = monitor->width;
    gushort mon_h = monitor->height;
    gshort mon_x = monitor->left_x;
    gshort mon_y = monitor->top_y;

    bool border_x, cut_x;
    gushort scaled_w, scaled_h;
    gushort margin_x, margin_y;

    RenderingRegion *rr = malloc(sizeof(RenderingRegion));

    img_w = MagickGetImageWidth(wand);
    img_h = MagickGetImageHeight(wand);

    /* For BG_MODE_CENTER, if the image is larger than the monitor,
       switch to BG_MODE_MAX to scale the image proportionally.*/
    if (bg_mode == BG_MODE_CENTER && (img_h > mon_h || img_w > mon_w)) {
        bg_mode = BG_MODE_MAX;
    }

    // Determine the rendering region based on the selected background mode
    switch (bg_mode) {
    case BG_MODE_SCALE:
        // BG_MODE_SCALE: Stretch the image to completely fill the monitor.
        rr->src_x = 0;
        rr->src_y = 0;
        rr->monitor_x = mon_x;
        rr->monitor_y = mon_y;
        rr->width = mon_w;
        rr->height = mon_h;
        break;
    case BG_MODE_CENTER:
        /* BG_MODE_CENTER: Place the image at its original size, centered on the
           monitor. */
        rr->src_x = 0;
        rr->src_y = 0;
        rr->monitor_x = mon_x + (mon_w - img_w) / 2;
        rr->monitor_y = mon_y + (mon_h - img_h) / 2;
        rr->width = img_w;
        rr->height = img_h;
        break;
    case BG_MODE_MAX:
        /* BG_MODE_MAX: Scale the image proportionally so that it fills the
           monitor. Determine if the image is limited by width (border_x true)
           or height. */
        rr->src_x = 0;
        rr->src_y = 0;
        border_x = img_w * mon_h < img_h * mon_w;
        // If border_x is true, scale based on height; otherwise, use full
        // monitor width.
        rr->width = border_x ? ((mon_h * img_w) / img_h) : mon_w;
        // Similarly, determine the height.
        rr->height = !border_x ? ((mon_w * img_h) / img_w) : mon_h;
        // Calculate margins to center the scaled image.
        margin_x = (mon_w - rr->width) / 2;
        margin_y = (mon_h - rr->height) / 2;
        rr->monitor_x = mon_x + (border_x ? margin_x : 0);
        rr->monitor_y = mon_y + (!border_x ? margin_y : 0);
        break;
    default:
        /* Default mode (Cropping):
           Scale the image so that one dimension exactly fits the monitor.
           The other dimension will exceed the monitor size, so it is cropped.
         */
        rr->monitor_x = mon_x;
        rr->monitor_y = mon_y;
        // Determine if we need to cut (crop) along the horizontal axis.
        cut_x = img_w * mon_h > img_h * mon_w;
        // Compute the scaled dimensions for both axes.
        scaled_w = (mon_h * img_w) / img_h;
        scaled_h = (mon_w * img_h) / img_w;
        // Set the final width and height of the rendering region.
        rr->width = cut_x ? scaled_w : mon_w;
        rr->height = cut_x ? mon_h : scaled_h;
        // Calculate the source offsets to center the crop.
        rr->src_x = cut_x ? ((scaled_w - mon_w) / 2) : 0;
        rr->src_y = !cut_x ? ((scaled_h - mon_h) / 2) : 0;
    }

    return rr;
}
