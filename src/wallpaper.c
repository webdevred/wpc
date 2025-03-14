/* wallpaper.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2020 Birte Kristina Friesel.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "monitors.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wand/MagickWand.h>
#include <wand/magick-image.h>

#include "rendering_region.h"
#include "structs.h"
#include "utils.h"
#include "wallpaper.h"

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
XContext xid_context = 0;
Window root = 0;

#if IMAGEMAGICK_MAJOR_VERSION >= 7
const char *pixel_format = "RGBA";
#else
const char *pixel_format = "BGRA";
#endif

void init_x(void) {
    init_disp(&disp, &root);
    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));

    scr = ScreenOfDisplay(disp, DefaultScreen(disp));
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    return;
}

static void set_default_backgroud(gchar *bg_fallback, Monitor *monitor,
                                  Pixmap pmap, GC *gc) {
    XGCValues gcval;
    XColor color;
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));

    XAllocNamedColor(disp, cmap, bg_fallback, &color, &color);
    gcval.foreground = color.pixel;
    *gc = XCreateGC(disp, root, GCForeground, &gcval);
    XFillRectangle(disp, pmap, *gc, monitor->left_x, monitor->top_y,
                   monitor->width, monitor->height);
}

static void set_bg_for_monitor_tiled(MagickWand *wand, GC *gc, Monitor *monitor,
                                     Pixmap pmap) {
    guint i, j;
    guint img_w = MagickGetImageWidth(wand);
    guint img_h = MagickGetImageHeight(wand);

    guint amount_x = (monitor->width + img_w - 1) / img_w;
    guint amount_y = (monitor->height + img_h - 1) / img_h;

    guint output_width = amount_x * img_w;
    guint output_height = amount_y * img_h;

    PixelWand *color = NewPixelWand();
    PixelSetColor(color, "none");

    MagickWand *tiled_wand = NewMagickWand();
    MagickNewImage(tiled_wand, output_width, output_height, color);

    for (i = 0; i < amount_x; i++) {
        for (j = 0; j < amount_y; j++) {
            MagickCompositeImage(tiled_wand, wand, OverCompositeOp,
                                 img_w * i, img_h * j);
        }
    }

    DestroyPixelWand(color);

    XGCValues gcval;

    gcval.foreground = None;
    *gc = XCreateGC(disp, root, GCForeground, &gcval);

    unsigned char *pixels =
        (unsigned char *)malloc(output_width * output_height * 4);

    MagickExportImagePixels(tiled_wand, 0, 0, output_width, output_height,
                            pixel_format, CharPixel, pixels);

    DestroyMagickWand(tiled_wand);
    XImage *ximage = XCreateImage(disp, vis, depth, ZPixmap, 0, (char *)pixels,
                                  output_width, output_height, 32, 0);

    XPutImage(disp, pmap, *gc, ximage, 0, 0, monitor->left_x, monitor->top_y,
              monitor->width, monitor->height);

    ximage->data = NULL;
    XDestroyImage(ximage);
    free(pixels);
    pixels = NULL;
}

static void set_bg_for_monitor_non_tiled(MagickWand *wand,
                                         gchar *conf_bg_fb_color,
                                         BgMode bg_mode, GC *gc,
                                         Monitor *monitor, Pixmap pmap) {
    gchar *bg_fallback_color;
    if (is_empty_string(conf_bg_fb_color)) {
        bg_fallback_color = g_strdup("#ff0000");
    } else {
        bg_fallback_color = g_strdup(conf_bg_fb_color);
    }

    set_default_backgroud(bg_fallback_color, monitor, pmap, gc);

    g_free(bg_fallback_color);

    RenderingRegion *rr = create_rendering_region(wand, monitor, bg_mode);

    MagickResizeImage(wand, rr->width, rr->height, LanczosFilter, 1.0);

    unsigned char *pixels = (unsigned char *)malloc(rr->width * rr->height * 4);

    MagickExportImagePixels(wand, 0, 0, rr->width, rr->height, pixel_format,
                            CharPixel, pixels);
    XImage *ximage = XCreateImage(disp, vis, depth, ZPixmap, 0, (char *)pixels,
                                  rr->width, rr->height, 32, 0);
    XPutImage(disp, pmap, *gc, ximage, rr->src_x, rr->src_y, rr->monitor_x,
              rr->monitor_y, rr->width, rr->height);

    free(rr);

    ximage->data = NULL;
    XDestroyImage(ximage);
    free(pixels);
    pixels = NULL;

    XFreeGC(disp, *gc);
}

static void set_bg_for_monitor(const gchar *wallpaper_path,
                               gchar *conf_bg_fb_color, BgMode bg_mode, GC *gc,
                               Monitor *monitor, Pixmap pmap) {
    MagickWandGenesis();

    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper_path) == MagickFalse) {
        DestroyMagickWand(wand);
        MagickWandTerminus();
        g_warning("Failed to read image: %s\n", wallpaper_path);
        return;
    }

    guint img_w = MagickGetImageWidth(wand);
    guint img_h = MagickGetImageHeight(wand);

    if (bg_mode == BG_MODE_TILE) {
        if (img_w <= monitor->width && img_h <= monitor->height) {
            set_bg_for_monitor_tiled(wand, gc, monitor, pmap);
        } else {
            g_warning("image %s too big for monitor %s with BG_MODE_TILE, "
                      "using BG_MODE_FILL instead",
                      wallpaper_path, monitor->name);
            set_bg_for_monitor_non_tiled(wand, conf_bg_fb_color, BG_MODE_FILL,
                                         gc, monitor, pmap);
        }
    } else {
        set_bg_for_monitor_non_tiled(wand, conf_bg_fb_color, bg_mode, gc,
                                     monitor, pmap);
    }

    DestroyMagickWand(wand);
    MagickWandTerminus();
    return;
}

static void feh_wm_set_bg(Config *config) {
    GC gc;
    XGCValues gcvalues;

    Atom prop_root, prop_esetroot, type;
    int format;
    unsigned long length, after;
    unsigned char *data_root = NULL, *data_esetroot = NULL;
    Pixmap pmap_d1, pmap_d2;

    Display *disp2;
    Window root2;
    gushort depth2;

    pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
    ArrayWrapper *mon_arr_wrapper = list_monitors(false);
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
                found = true;
                break;
            }
        }

        if (!found) continue;

        g_info("set filled bg: %s %s %d %d %d %d", monitor->name,
               wallpaper_path, monitor->width, monitor->height, monitor->left_x,
               monitor->top_y);

        set_bg_for_monitor(wallpaper_path, bg_fallback_color, bg_mode, &gc,
                           monitor, pmap_d1);
    }
    free_monitors(mon_arr_wrapper);

    /* create new display, copy pixmap to new display */
    disp2 = XOpenDisplay(NULL);
    if (!disp2) g_error("Can't reopen X display.");
    root2 = RootWindow(disp2, DefaultScreen(disp2));
    depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
    XSync(disp, False);
    pmap_d2 = XCreatePixmap(disp2, root2, scr->width, scr->height, depth2);
    gcvalues.fill_style = FillTiled;
    gcvalues.tile = pmap_d1;
    gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(disp2, pmap_d2, gc, 0, 0, scr->width, scr->height);
    XFreeGC(disp2, gc);
    XSync(disp2, False);
    XSync(disp, False);
    XFreePixmap(disp, pmap_d1);

    prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", True);
    prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", True);

    if (prop_root != None && prop_esetroot != None) {
        XGetWindowProperty(disp2, root2, prop_root, 0L, 1L, False,
                           AnyPropertyType, &type, &format, &length, &after,
                           &data_root);
        if (type == XA_PIXMAP) {
            XGetWindowProperty(disp2, root2, prop_esetroot, 0L, 1L, False,
                               AnyPropertyType, &type, &format, &length, &after,
                               &data_esetroot);
            if (data_root && data_esetroot) {
                if (type == XA_PIXMAP &&
                    *((Pixmap *)data_root) == *((Pixmap *)data_esetroot)) {
                    XKillClient(disp2, *((Pixmap *)data_root));
                }
            }
        }
    }

    if (data_root) XFree(data_root);

    if (data_esetroot) XFree(data_esetroot);

    /* This will locate the property, creating it if it doesn't exist */
    prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
    prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

    if (prop_root == None || prop_esetroot == None)
        g_error("creation of pixmap property failed.");

    XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&pmap_d2, 1);
    XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&pmap_d2, 1);

    XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
    XClearWindow(disp2, root2);
    XFlush(disp2);
    XSetCloseDownMode(disp2, RetainPermanent);
    XCloseDisplay(disp2);

    return;
}

extern void set_wallpapers(Config *config) {
    init_x();
    feh_wm_set_bg(config);
}
