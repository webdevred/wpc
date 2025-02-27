#include "rendering_region.h"

extern RenderingRegion *
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode) {
    gushort img_w, img_h;

    gushort mon_w = monitor->width;
    gushort mon_h = monitor->height;
    gushort mon_x = monitor->horizontal_position;
    gushort mon_y = monitor->vertical_position;

    bool border_x, cut_x;
    gushort scaled_w, scaled_h, margin_x, margin_y;

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
        rr->src_x = 0;
        rr->src_y = 0;
        rr->monitor_x = mon_x + ((mon_w - img_w) >> 1);
        rr->monitor_y = mon_y + ((mon_h - img_h) >> 1);
        rr->width = img_w;
        rr->height = img_h;
        break;
    case BG_MODE_MAX:
        rr->src_x = 0;
        rr->src_y = 0;
        border_x = ((img_w * mon_h) > (img_h * mon_w)) ? false : true;
        rr->width = border_x ? ((mon_h * img_w) / img_h) : mon_w;
        rr->height = !border_x ? ((mon_w * img_h) / img_w) : mon_h;
        margin_x = (mon_w - rr->width) >> 1;
        margin_y = (mon_h - rr->height) >> 1;
        rr->monitor_x = mon_x + (border_x ? margin_x : 0);
        rr->monitor_y = mon_y + (!border_x ? margin_y : 0);
        break;
    default:
        rr->monitor_x = mon_x;
        rr->monitor_y = mon_y;

        cut_x = img_w * mon_h > img_h * mon_w ? true : false;

        scaled_w = (mon_w * img_h) / img_w;
        scaled_h = (mon_h * img_w) / img_h;
        rr->width = cut_x ? mon_w : scaled_w;
        rr->height = cut_x ? scaled_h : mon_h;

        rr->src_x = cut_x ? ((scaled_w - rr->width) >> 1) : 0;
        rr->src_y = !cut_x ? ((scaled_h - rr->height) >> 1) : 0;
    }

    return rr;
}
