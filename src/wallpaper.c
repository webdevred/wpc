// Copyright 2025 webdevred

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <string.h>

#include "wpc/filesystem.h"
#include "wpc/monitors.h"
#include "wpc/wallpaper.h"
#include "wpc/wallpaper_transformation.h"

#include "wpc/wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

#ifdef WPC_IMAGEMAGICK_7
const static char *pixel_format = "BGRA";
#else
const static char *pixel_format = "RGBA";
#endif

static void set_bg_for_monitor(const gchar *wallpaper_path,
                               gchar *conf_bg_fb_color, BgMode bg_mode,
                               Monitor *monitor, Pixmap pmap) {
    unsigned char *pixels;
    XImage *ximage;
    GC gc;
    XGCValues gcval;
    MagickWand *wand;

    wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper_path) == MagickFalse) {
        DestroyMagickWand(wand);
        g_warning("Failed to read image: %s\n", wallpaper_path);
        return;
    }

    if (bg_mode != BG_MODE_TILE || !transform_wallpaper_tiled(&wand, monitor)) {
        transform_wallpaper(&wand, monitor, bg_mode, conf_bg_fb_color);
    }

    pixels = (unsigned char *)malloc(monitor->width * monitor->height * 4);

    MagickExportImagePixels(wand, 0, 0, monitor->width, monitor->height,
                            pixel_format, CharPixel, pixels);
    ximage = XCreateImage(querying_display, rendering_visual,
                          (guint)querying_depth, ZPixmap, 0, (char *)pixels,
                          (guint)monitor->width, (guint)monitor->height, 32, 0);

    gcval.foreground = None;
    gc = XCreateGC(querying_display, querying_root, GCForeground, &gcval);

    XPutImage(querying_display, pmap, gc, ximage, 0, 0, (gint)monitor->left_x,
              (gint)monitor->top_y, (guint)monitor->width,
              (guint)monitor->height);

    ximage->data = NULL;
    XDestroyImage(ximage);
    free(pixels);
    pixels = NULL;

    XFreeGC(querying_display, gc);

    DestroyMagickWand(wand);
    return;
}

static Atom get_atom(Display *display, char *atom_name, Bool only_if_exists) {
    Atom atom = XInternAtom(display, atom_name, only_if_exists);
    if (atom == None) {
        g_error("failed to get atom %s", atom_name);
    }
    return atom;
}

extern void set_wallpapers(Config *config, WallpaperQueue *queue,
                           MonitorArray *mon_arr_wrapper) {
    Monitor *monitors;
    ConfigMonitor *monitor_bgs;
    gushort m;
    Atom prop_root, prop_esetroot;
    Pixmap pmap_d1, pmap_d2;
    GC gc;
    BgMode bg_mode;
    gushort w;
    bool found;
    Monitor *monitor;
    XGCValues gcvalues;

    gchar *wallpaper_path, *bg_fallback_color;

    pmap_d1 = XCreatePixmap(
        querying_display, querying_root, (guint)rendering_screen->width,
        (guint)rendering_screen->height, (guint)querying_depth);
    monitors = (Monitor *)mon_arr_wrapper->data;
    monitor_bgs = config->monitors_with_backgrounds;
    wallpaper_path = NULL;
    bg_fallback_color = NULL;

    bg_mode = BG_MODE_FILL;

    for (m = 0; m < mon_arr_wrapper->amount_used; m++) {
        monitor = &monitors[m];

        found = false;
        for (w = 0; w < config->number_of_monitors; w++) {
            if (strcmp(monitor->name, monitor_bgs[w].name) == 0) {
                wallpaper_path = monitor_bgs[w].image_path;
                bg_fallback_color = monitor_bgs[w].valid_bg_fallback_color;
                bg_mode = monitor_bgs[w].bg_mode;
                found = TRUE;
                break;
            }
        }

        if (!found) {
            if (queue != NULL) {
                wallpaper_path = next_wallpaper_in_queue(queue);
                if (wallpaper_path == NULL) continue;
                g_info("couldnt find configured wallapaper, selected %s",
                       wallpaper_path);
            } else {
                continue;
            }
        }

        g_info("set filled bg: %s %s %d %d %d %d", monitor->name,
               wallpaper_path, monitor->width, monitor->height, monitor->left_x,
               monitor->top_y);

        set_bg_for_monitor(wallpaper_path, bg_fallback_color, bg_mode, monitor,
                           pmap_d1);
    }

    /* create new display, copy pixmap to new display */
    XSync(querying_display, False);
    pmap_d2 = XCreatePixmap(
        rendering_display, rendering_root, (guint)rendering_screen->width,
        (guint)rendering_screen->height, (guint)rendering_depth);
    gcvalues = (XGCValues){
        .fill_style = FillTiled,
        .tile = pmap_d1,
    };
    gc = XCreateGC(rendering_display, pmap_d2, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(rendering_display, pmap_d2, gc, 0, 0,
                   (guint)rendering_screen->width,
                   (guint)rendering_screen->height);

    prop_root = get_atom(rendering_display, "_XROOTPMAP_ID", False);
    prop_esetroot = get_atom(rendering_display, "ESETROOT_PMAP_ID", False);

    if (prop_root == None || prop_esetroot == None) {
        g_critical("creation of pixmap property failed.");
        goto cleanup;
    }

    XChangeProperty(rendering_display, rendering_root, prop_root, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *)&pmap_d2, 1);
    XChangeProperty(rendering_display, rendering_root, prop_esetroot, XA_PIXMAP,
                    32, PropModeReplace, (unsigned char *)&pmap_d2, 1);

    XSetWindowBackgroundPixmap(rendering_display, rendering_root, pmap_d2);
    XClearWindow(rendering_display, rendering_root);
    XFlush(rendering_display);
    XSetCloseDownMode(rendering_display, RetainPermanent);

cleanup:
    XFreeGC(rendering_display, gc);
    XSync(rendering_display, False);
    XSync(querying_display, False);
    XFreePixmap(querying_display, pmap_d1);

    return;
}
