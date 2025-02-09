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

#include "config.h"
#include "feh.h"
#include "filesystem.h"
#include "wallpaper.h"

Window ipc_win = None;
Window my_ipc_win = None;
Atom ipc_atom = None;

/*
 * This is a boolean indicating
 * That while we seem to see E16 IPC
 * it's actually E17 faking it
 * -- richlowe 2005-06-22
 */

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

/*
** Creates a script that can be used to create the same background
** as the last time the program was called.
*/

static void feh_wm_set_bg(enum bgmode_type bgmode,
                          MonitorBackgroundPair *monitors,
                          int number_of_monitors) {
    XGCValues gcvalues;
    XGCValues gcval;
    GC gc;

    /*
     * TODO this re-implements mkstemp (badly). However, it is only needed
     * for non-file images and enlightenment. Might be easier to just remove
     * it.
     */

    Atom prop_root, prop_esetroot, type;
    int format;
    unsigned long length, after;
    unsigned char *data_root = NULL, *data_esetroot = NULL;
    Pixmap pmap_d1, pmap_d2;
    Imlib_Image im;

    /* local display to set closedownmode on */
    Display *disp2;
    Window root2;
    int depth2;

    D(("Falling back to XSetRootWindowPixmap\n"));

    XColor color;
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));

    XAllocNamedColor(disp, cmap, "black", &color, &color);

    if (bgmode == BG_MODE_SCALE) {
        pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);

#ifdef HAVE_LIBXINERAMA
        if (opt.xinerama_index >= 0) {
            gcval.foreground = color.pixel;
            gc = XCreateGC(disp, root, GCForeground, &gcval);
            XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);
            XFreeGC(disp, gc);
        }

        if (opt.xinerama && xinerama_screens) {
            for (i = 0; i < num_xinerama_screens; i++) {
                if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
                    feh_wm_set_bg_scaled(
                        pmap_d1, im, use_filelist, xinerama_screens[i].x_org,
                        xinerama_screens[i].y_org, xinerama_screens[i].width,
                        xinerama_screens[i].height);
                }
            }
        } else
#endif /* HAVE_LIBXINERAMA */
            feh_wm_set_bg_scaled(pmap_d1, im, 0, 0, scr->width, scr->height);
    } else if (bgmode == BG_MODE_CENTER) {

        D(("centering\n"));

        pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
        gcval.foreground = color.pixel;
        gc = XCreateGC(disp, root, GCForeground, &gcval);
        XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);

#ifdef HAVE_LIBXINERAMA
        if (opt.xinerama && xinerama_screens) {
            for (i = 0; i < num_xinerama_screens; i++) {
                if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
                    feh_wm_set_bg_centered(
                        pmap_d1, im, use_filelist, xinerama_screens[i].x_org,
                        xinerama_screens[i].y_org, xinerama_screens[i].width,
                        xinerama_screens[i].height);
                }
            }
        } else
#endif /* HAVE_LIBXINERAMA */
            feh_wm_set_bg_centered(pmap_d1, im, 0, 0, scr->width, scr->height);

        XFreeGC(disp, gc);

    } else if (bgmode == BG_MODE_FILL) {
        pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
        MonitorBackgroundPair *end = monitors + number_of_monitors;
        while (monitors < end) {
            Monitor *monitor = get_monitor(monitors->name);

            char *wallpaper_path = monitors->image_path;
            im = imlib_load_image(wallpaper_path);

            printf("set filled bg: %s %s %d %d %d %d\n", monitor->name,
                   wallpaper_path, monitor->width, monitor->height,
                   monitor->horizontal_position, monitor->vertical_position);

            feh_wm_set_bg_filled(pmap_d1, im, monitor->horizontal_position,
                                 monitor->vertical_position, monitor->width,
                                 monitor->height);

            imlib_context_set_image(im);
            imlib_free_image_and_decache();
            monitors++;
        }
    }

    /* create new display, copy pixmap to new display */
    disp2 = XOpenDisplay(NULL);
    if (!disp2) eprintf("Can't reopen X display.");
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
        eprintf("creation of pixmap property failed.");

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
    MonitorBackgroundPair *monitors = config->monitors_with_backgrounds;
    int number_of_monitors = config->number_of_monitors;
    feh_wm_set_bg(BG_MODE_FILL, monitors, number_of_monitors);
    free_config(config);
}
