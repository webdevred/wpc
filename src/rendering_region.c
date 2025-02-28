#include "rendering_region.h"

extern RenderingRegion *
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode) {
    gushort img_w, img_h;

    gushort mon_w = monitor->width;
    gushort mon_h = monitor->height;
    gushort mon_x = monitor->horizontal_position;
    gushort mon_y = monitor->vertical_position;

    bool border_x, cut_x;
    gushort margin_x, margin_y;

    RenderingRegion *rr = malloc(sizeof(RenderingRegion));
    img_w = MagickGetImageWidth(wand);
    img_h = MagickGetImageHeight(wand);

    if (bg_mode == BG_MODE_CENTER && (img_h > mon_h || img_h > mon_h)) {
        bg_mode = BG_MODE_MAX;
    }

    switch (bg_mode) {
    case BG_MODE_SCALE:
        rr->src_x = 0;
        rr->src_y = 0;
        rr->monitor_x = mon_x;
        rr->monitor_y = mon_y;
        rr->width = mon_w;
        rr->height = mon_h;
        break;
    case BG_MODE_CENTER:
      /* add a margin to the center so that it is placed in the center of the screen */
        rr->src_x = 0;
        rr->src_y = 0;
        rr->monitor_x = mon_x + (mon_w - img_w) / 2;
        rr->monitor_y = mon_y + (mon_h - img_h) / 2;
        rr->width = img_w;
        rr->height = img_h;
        break;
    case BG_MODE_MAX:
      /* add a margin to the center so that it is placed in the center of the screen
         comparing products (old_height * new_width) and (old_height * new_width) to determine which side to scale
       also the side that doesnt need scaling have its size var (rr->width/rr->height) set to the monitor size for that side
      for the other side scale up according  to new_width = (new_height * old_wihth) / old_height */
        rr->src_x = 0;
        rr->src_y = 0;
        border_x = img_w * mon_h < img_h * mon_w;
        rr->width = border_x ? ((mon_h * img_w) / img_h) : mon_w;
        rr->height = !border_x ? ((mon_w * img_h) / img_w) : mon_h;
        margin_x = (mon_w - rr->width) / 2;
        margin_y = (mon_h - rr->height) / 2;
        rr->monitor_x = mon_x + (border_x ? margin_x : 0);
        rr->monitor_y = mon_y + (!border_x ? margin_y : 0);
        break;
    default:
      /*  comparing products (old_height * new_width) and (old_height * new_width) to determine which side to cut
        the side that doesnt need scaling have its size var (rr->width/rr->height) set to the monitor size for that side
      for the other side scale up according  to new_width = (new_height * old_width) / old_height */
        rr->monitor_x = mon_x;
        rr->monitor_y = mon_y;

        cut_x = img_w * mon_h > img_h * mon_w;

        rr->width = mon_w;
        rr->height = mon_h;

        rr->src_x = cut_x ? ( (rr->width - mon_w) / 2) : 0;
        rr->src_y = !cut_x ? ( (rr->height - mon_h) / 2) : 0;
    }

    return rr;
}
