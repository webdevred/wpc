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

#include <limits.h>
#include <sys/stat.h>

#include "monitors.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <wand/MagickWand.h>

#include "common.h"
#include "config.h"
#include "filesystem.h"
#include "imagemagick.h"
#include "wallpaper.h"
#include "wpc.h"
Window ipc_win = None;
Window my_ipc_win = None;
Atom ipc_atom = None;

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
XContext xid_context = 0;
Window root = 0;

int childpid = 0;

void init_x(void) {
    init_disp(&disp, &root);
    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));

    scr = ScreenOfDisplay(disp, DefaultScreen(disp));
    xid_context = XUniqueContext();
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    return;
}

static void set_default_backgroud(Monitor *monitor, Pixmap pmap, GC *gc) {
    XGCValues gcval;
    XColor color;
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));

    XAllocNamedColor(disp, cmap, "red", &color, &color);
    gcval.foreground = color.pixel;
    *gc = XCreateGC(disp, root, GCForeground, &gcval);
    XFillRectangle(disp, pmap, *gc, monitor->horizontal_position,
                   monitor->vertical_position, monitor->width, monitor->height);
}

typedef struct {
    int width;
    int height;
    int src_x;
    int src_y;
    int monitor_x;
    int monitor_y;
} RenderingRegion;

static RenderingRegion *
create_rendering_region(MagickWand *wand, Monitor *monitor, BgMode bg_mode) {
    int img_w, img_h;

    int mon_w = monitor->width;
    int mon_h = monitor->height;
    int mon_x = monitor->horizontal_position;
    int mon_y = monitor->vertical_position;

    int scaled_w, scaled_h;

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
        int x = (mon_w - img_w) >> 1;
        int y = (mon_h - img_h) >> 1;
        rr->src_x = 0;
        rr->src_y = 0;
        rr->monitor_x = x;
        rr->monitor_y = y;
        rr->width = img_w;
        rr->height = img_h;
        break;
    case BG_MODE_MAX:
        rr->src_x = 0;
        rr->src_y = 0;
        bool border_x = (((img_w * mon_h) > (img_h * mon_w)) ? false : true);
        rr->width = (border_x ? ((mon_h * img_w) / img_h) : mon_w);
        rr->height = (!border_x ? ((mon_w * img_h) / img_w) : mon_h);
        int margin_x = (mon_w - rr->width) >> 1;
        int margin_y = (mon_h - rr->height) >> 1;
        rr->monitor_x = mon_x + (border_x ? margin_x : 0);
        rr->monitor_y = mon_y + (!border_x ? margin_y : 0);
        break;
    default:
        rr->monitor_x = mon_x;
        rr->monitor_y = mon_y;

        bool cut_x = img_w * mon_h > img_h * mon_w ? true : false;

        scaled_w = ((mon_h * img_w) / img_h);
        scaled_h = ((mon_w * img_h) / img_w);
        rr->width = cut_x ? scaled_w : mon_w;
        rr->height = !cut_x ? scaled_h : mon_h;

        rr->src_x = (cut_x ? ((scaled_w - rr->width) >> 1) : 0);
        rr->src_y = (!cut_x ? ((scaled_h - rr->height) >> 1) : 0);
    }

    return rr;
}

static void set_bg_for_monitor(const gchar *wallpaper_path, BgMode bg_mode,
                               GC *gc, Monitor *monitor, Pixmap pmap) {
    set_default_backgroud(monitor, pmap, gc);

    MagickWandGenesis();

    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper_path) == MagickFalse) {
        fprintf(stderr, "Failed to read image: %s\n", wallpaper_path);
    }

    RenderingRegion *rr = create_rendering_region(wand, monitor, bg_mode);

    MagickResizeImage(wand, rr->width, rr->height, LanczosFilter, 1.0);
    unsigned char *pixels = (unsigned char *)malloc(rr->width * rr->height * 4);

    MagickExportImagePixels(wand, 0, 0, rr->width, rr->height, "BGRA",
                            CharPixel, pixels);
    XImage *ximage = XCreateImage(disp, vis, depth, ZPixmap, 0, (char *)pixels,
                                  rr->width, rr->height, 32, 0);

    printf("%d %d %d %d %d %d\n", rr->src_x, rr->src_y, rr->monitor_x,
           rr->monitor_y, rr->width, rr->height);
    XPutImage(disp, pmap, *gc, ximage, rr->src_x, rr->src_y, rr->monitor_x,
              rr->monitor_y, rr->width, rr->height);

    free(rr);
    XFreeGC(disp, *gc);
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
    int depth2;

    pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
    ArrayWrapper *mon_arr_wrapper = list_monitors(true);
    Monitor *monitors = (Monitor *)mon_arr_wrapper->data;
    unsigned int m;
    MonitorBackgroundPair *monitor_bgs = config->monitors_with_backgrounds;
    char *wallpaper_path;
    BgMode bg_mode;

    for (m = 0; m < mon_arr_wrapper->amount_used; m++) {
        gushort w;
        Monitor *monitor = &monitors[m];

        bool found = false;
        for (w = 0; w < config->number_of_monitors; w++) {
            if (strcmp(monitor->name, monitor_bgs[w].name) == 0) {
                wallpaper_path = monitor_bgs[w].image_path;
                bg_mode = monitor_bgs[w].bg_mode;
                found = true;
                break;
            }
        }

        if (!found) continue;

        g_info("set filled bg: %s %s %d %d %d %d", monitor->name,
               wallpaper_path, monitor->width, monitor->height,
               monitor->horizontal_position, monitor->vertical_position);

        set_bg_for_monitor(wallpaper_path, bg_mode, &gc, monitor, pmap_d1);
    }

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

extern void set_wallpapers() {
    init_x();
    Config *config = load_config();
    feh_wm_set_bg(config);
    free_config(config);
}
