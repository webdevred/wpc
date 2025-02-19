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
#include <Imlib2.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "common.h"
#include "config.h"
#include "filesystem.h"
#include "gib_imlib.h"
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

void init_x_and_imlib(void) {
    init_disp(&disp, &root);
    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));

    scr = ScreenOfDisplay(disp, DefaultScreen(disp));
    xid_context = XUniqueContext();

    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_color_modifier(NULL);
    imlib_context_set_progress_function(NULL);
    imlib_context_set_operation(IMLIB_OP_COPY);
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    imlib_set_cache_size(1024 * 1024);

    return;
}

static void feh_wm_set_bg_scaled(Pixmap pmap, Imlib_Image im, int x, int y,
                                 int w, int h) {
    gib_imlib_render_image_on_drawable_at_size(pmap, im, x, y, w, h, 1, 1, 1);

    return;
}

static void feh_wm_set_bg_centered(Pixmap pmap, Imlib_Image im, int x, int y,
                                   int w, int h) {
    int offset_x, offset_y;

    offset_x = (w - gib_imlib_image_get_width(im)) >> 1;
    offset_y = (h - gib_imlib_image_get_height(im)) >> 1;

    gib_imlib_render_image_part_on_drawable_at_size(
        pmap, im, ((offset_x < 0) ? -offset_x : 0),
        ((offset_y < 0) ? -offset_y : 0), w, h,
        x + ((offset_x > 0) ? offset_x : 0),
        y + ((offset_y > 0) ? offset_y : 0), w, h, 1, 1, 0);

    return;
}

static void feh_wm_set_bg_filled(Pixmap pmap, Imlib_Image im, int x, int y,
                                 int w, int h) {
    int img_w, img_h, cut_x;
    int render_w, render_h, render_x, render_y;

    img_w = gib_imlib_image_get_width(im);
    img_h = gib_imlib_image_get_height(im);

    cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

    render_w = (cut_x ? ((img_h * w) / h) : img_w);
    render_h = (!cut_x ? ((img_w * h) / w) : img_h);

    render_x = (cut_x ? ((img_w - render_w) >> 1) : 0);
    render_y = (!cut_x ? ((img_h - render_h) >> 1) : 0);

    gib_imlib_render_image_part_on_drawable_at_size(
        pmap, im, render_x, render_y, render_w, render_h, x, y, w, h, 1, 1, 1);

    return;
}

static void feh_wm_set_bg_maxed(Pixmap pmap, Imlib_Image im, int x, int y,
                                int w, int h) {
    int img_w, img_h, border_x;
    int render_w, render_h, render_x, render_y;
    int margin_x, margin_y;

    img_w = gib_imlib_image_get_width(im);
    img_h = gib_imlib_image_get_height(im);

    border_x = (((img_w * h) > (img_h * w)) ? 0 : 1);

    render_w = (border_x ? ((img_w * h) / img_h) : w);
    render_h = (!border_x ? ((img_h * w) / img_w) : h);

    margin_x = (w - render_w) >> 1;
    margin_y = (h - render_h) >> 1;

    render_x = x + (border_x ? margin_x : 0);
    render_y = y + (!border_x ? margin_y : 0);

    gib_imlib_render_image_on_drawable_at_size(pmap, im, render_x, render_y,
                                               render_w, render_h, 1, 1, 1);

    return;
}

static void set_default_backgroud(Monitor *monitor, Pixmap pmap, GC *gc) {
    XGCValues gcval;
    XColor color;
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));

    XAllocNamedColor(disp, cmap, "black", &color, &color);
    gcval.foreground = color.pixel;
    *gc = XCreateGC(disp, root, GCForeground, &gcval);
    XFillRectangle(disp, pmap, *gc, monitor->horizontal_position,
                   monitor->vertical_position, monitor->width, monitor->height);
}

static bool is_image_bigger_than_monitor(Monitor *monitor,
                                         char *wallpaper_path) {
    Wallpaper *wallpaper = malloc(sizeof(Wallpaper));
    set_resolution(wallpaper_path, wallpaper);
    if (monitor->width > wallpaper->width ||
        monitor->height > wallpaper->height) {
        return false;
    }
    free(wallpaper);
    return true;
}

static void feh_wm_set_bg(Config *config) {
    XGCValues gcvalues;
    GC gc;

    Atom prop_root, prop_esetroot, type;
    int format;
    unsigned long length, after;
    unsigned char *data_root = NULL, *data_esetroot = NULL;
    Pixmap pmap_d1, pmap_d2;
    Imlib_Image im;

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
        ushort w;
        Monitor *monitor = &monitors[m];

        for (w = 0; w < config->number_of_monitors; w++) {
            if (strcmp(monitor->name, monitor_bgs[w].name) == 0) {
                wallpaper_path = monitor_bgs[w].image_path;
                bg_mode = monitor_bgs[w].bg_mode;
                break;
            }
        }

        if (bg_mode == BG_MODE_CENTER &&
            is_image_bigger_than_monitor(monitor, wallpaper_path)) {
            bg_mode = BG_MODE_MAX;
        }

        im = imlib_load_image(wallpaper_path);

        g_info("set filled bg: %s %s %d %d %d %d", monitor->name,
               wallpaper_path, monitor->width, monitor->height,
               monitor->horizontal_position, monitor->vertical_position);
        switch (bg_mode) {
        case BG_MODE_SCALE:
            feh_wm_set_bg_scaled(pmap_d1, im, monitor->horizontal_position,
                                 monitor->vertical_position, monitor->width,
                                 monitor->height);
            break;
        case BG_MODE_MAX:
            set_default_backgroud(monitor, pmap_d1, &gc);
            feh_wm_set_bg_maxed(pmap_d1, im, monitor->horizontal_position,
                                monitor->vertical_position, monitor->width,
                                monitor->height);
            XFreeGC(disp, gc);
            break;
        case BG_MODE_CENTER:
            set_default_backgroud(monitor, pmap_d1, &gc);
            feh_wm_set_bg_centered(pmap_d1, im, monitor->horizontal_position,
                                   monitor->vertical_position, monitor->width,
                                   monitor->height);
            XFreeGC(disp, gc);
            break;
        default:
            feh_wm_set_bg_filled(pmap_d1, im, monitor->horizontal_position,
                                 monitor->vertical_position, monitor->width,
                                 monitor->height);
            break;
        }

        imlib_context_set_image(im);
        imlib_free_image_and_decache();
    }

    /* create new display, copy pixmap to new display */
    disp2 = XOpenDisplay(NULL);
    if (!disp2) logprintf(ERROR, "Can't reopen X display.");
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
        logprintf(ERROR, "creation of pixmap property failed.");

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
    init_x_and_imlib();
    Config *config = load_config();
    feh_wm_set_bg(config);
    free_config(config);
}
