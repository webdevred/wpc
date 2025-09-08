#include "filesystem.h"
#include "glib.h"
#include "monitors.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>

#include "wallpaper.h"
#include "wallpaper_transformation.h"

#include "wpc_imagemagick.h"
__attribute__((used)) static void _mark_magick_used(void) {
    _wpc_magick_include_marker();
}

Atom wmDeleteWindow;
XContext xid_context = 0;

#ifdef WPC_IMAGEMAGICK_7
const char *pixel_format = "BGRA";
#else
const char *pixel_format = "RGBA";
#endif

static void set_bg_for_monitor(const gchar *wallpaper_path,
                               gchar *conf_bg_fb_color, BgMode bg_mode,
                               Monitor *monitor, Pixmap pmap) {
    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper_path) == MagickFalse) {
        DestroyMagickWand(wand);
        g_warning("Failed to read image: %s\n", wallpaper_path);
        return;
    }

    if (bg_mode != BG_MODE_TILE || !transform_wallpaper_tiled(&wand, monitor)) {
        transform_wallpaper(&wand, monitor, bg_mode, conf_bg_fb_color);
    }

    unsigned char *pixels =
        (unsigned char *)malloc(monitor->width * monitor->height * 4);

    MagickExportImagePixels(wand, 0, 0, monitor->width, monitor->height,
                            pixel_format, CharPixel, pixels);
    XImage *ximage = XCreateImage(querying_display, rendering_visual,
                                  querying_depth, ZPixmap, 0, (char *)pixels,
                                  monitor->width, monitor->height, 32, 0);

    XGCValues gcval;
    gcval.foreground = None;
    GC gc = XCreateGC(querying_display, querying_root, GCForeground, &gcval);

    XPutImage(querying_display, pmap, gc, ximage, 0, 0, monitor->left_x,
              monitor->top_y, monitor->width, monitor->height);

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
    GC gc;

    Atom prop_root, prop_esetroot;
    Pixmap pmap_d1, pmap_d2;

    pmap_d1 =
        XCreatePixmap(querying_display, querying_root, rendering_screen->width,
                      rendering_screen->height, querying_depth);
    Monitor *monitors = (Monitor *)mon_arr_wrapper->data;
    gushort m;
    ConfigMonitor *monitor_bgs = config->monitors_with_backgrounds;
    gchar *wallpaper_path = NULL;
    gchar *bg_fallback_color = NULL;
    BgMode bg_mode;

    gushort w;
    bool found;
    Monitor *monitor;

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
    pmap_d2 = XCreatePixmap(rendering_display, rendering_root,
                            rendering_screen->width, rendering_screen->height,
                            rendering_depth);
    XGCValues gcvalues = {
        .fill_style = FillTiled,
        .tile = pmap_d1,
    };
    gc = XCreateGC(rendering_display, pmap_d2, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(rendering_display, pmap_d2, gc, 0, 0,
                   rendering_screen->width, rendering_screen->height);

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
