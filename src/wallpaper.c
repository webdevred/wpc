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

#include "feh.h"
#include "filelist.h"
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

extern void feh_wm_set_bg_filelist(unsigned char bgmode) {
    if (filelist_len == 0) eprintf("No files specified for background setting");

    switch (bgmode) {
    case BG_MODE_TILE:
        feh_wm_set_bg(NULL, NULL, 0, 0, 0, 0, 1);
        break;
    case BG_MODE_SCALE:
        feh_wm_set_bg(NULL, NULL, 0, 1, 0, 0, 1);
        break;
    case BG_MODE_FILL:
        feh_wm_set_bg(NULL, NULL, 0, 0, 1, 0, 1);
        break;
    case BG_MODE_MAX:
        feh_wm_set_bg(NULL, NULL, 0, 0, 2, 0, 1);
        break;
    default:
        feh_wm_set_bg(NULL, NULL, 1, 0, 0, 0, 1);
        break;
    }
}

static void feh_wm_load_next(Imlib_Image *im) {
    static gib_list *wpfile = NULL;

    if (wpfile == NULL) wpfile = filelist;

    if (feh_load_image(im, FEH_FILE(wpfile->data)) == 0)
        eprintf("Unable to load image %s", FEH_FILE(wpfile->data)->filename);
    if (wpfile->next) wpfile = wpfile->next;

    return;
}

static void feh_wm_set_bg_scaled(Pixmap pmap, Imlib_Image im, int use_filelist,
                                 int x, int y, int w, int h) {
    if (use_filelist) feh_wm_load_next(&im);

    gib_imlib_render_image_on_drawable_at_size(pmap, im, x, y, w, h, 1, 1, 1);

    if (use_filelist) gib_imlib_free_image_and_decache(im);

    return;
}

static void feh_wm_set_bg_centered(Pixmap pmap, Imlib_Image im,
                                   int use_filelist, int x, int y, int w,
                                   int h) {
    int offset_x, offset_y;

    if (use_filelist) feh_wm_load_next(&im);

    offset_x = (w - gib_imlib_image_get_width(im)) >> 1;
    offset_y = (h - gib_imlib_image_get_height(im)) >> 1;

    gib_imlib_render_image_part_on_drawable_at_size(
        pmap, im, ((offset_x < 0) ? -offset_x : 0),
        ((offset_y < 0) ? -offset_y : 0), w, h,
        x + ((offset_x > 0) ? offset_x : 0),
        y + ((offset_y > 0) ? offset_y : 0), w, h, 1, 1, 0);

    if (use_filelist) gib_imlib_free_image_and_decache(im);

    return;
}

static void feh_wm_set_bg_filled(Pixmap pmap, Imlib_Image im, int use_filelist,
                                 int x, int y, int w, int h) {
    int img_w, img_h, cut_x;
    int render_w, render_h, render_x, render_y;

    if (use_filelist) feh_wm_load_next(&im);

    img_w = gib_imlib_image_get_width(im);
    img_h = gib_imlib_image_get_height(im);

    cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

    render_w = (cut_x ? ((img_h * w) / h) : img_w);
    render_h = (!cut_x ? ((img_w * h) / w) : img_h);

    render_x = (cut_x ? ((img_w - render_w) >> 1) : 0);
    render_y = (!cut_x ? ((img_h - render_h) >> 1) : 0);

    gib_imlib_render_image_part_on_drawable_at_size(
        pmap, im, render_x, render_y, render_w, render_h, x, y, w, h, 1, 1, 1);

    if (use_filelist) gib_imlib_free_image_and_decache(im);

    return;
}

static void feh_wm_set_bg_maxed(Pixmap pmap, Imlib_Image im, int use_filelist,
                                int x, int y, int w, int h) {
    int img_w, img_h, border_x;
    int render_w, render_h, render_x, render_y;
    int margin_x, margin_y;

    if (use_filelist) feh_wm_load_next(&im);

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

    if (use_filelist) gib_imlib_free_image_and_decache(im);

    return;
}

/*
** Creates a script that can be used to create the same background
** as the last time the program was called.
*/

static void feh_wm_set_bg(char *fil, Imlib_Image im, int centered, int scaled,
                          int filled, int desktop, int use_filelist) {
    XGCValues gcvalues;
    XGCValues gcval;
    GC gc;
    char bgname[20];
    int num = 0;
    ;
    char bgfil[4096];

    /*
     * TODO this re-implements mkstemp (badly). However, it is only needed
     * for non-file images and enlightenment. Might be easier to just remove
     * it.
     */

    snprintf(bgname, sizeof(bgname), "FEHBG_%d", num);

    if (!fil && im) {
        if (getenv("HOME") == NULL) {
            weprintf(
                "Cannot save wallpaper to temporary file: You have no HOME");
            return;
        }
        snprintf(bgfil, sizeof(bgfil), "%s/.%s.png", getenv("HOME"), bgname);
        imlib_context_set_image(im);
        imlib_image_set_format("png");
        gib_imlib_save_image(im, bgfil);
        D(("bg saved as %s\n", bgfil));
        fil = bgfil;
    }

    Atom prop_root, prop_esetroot, type;
    int format;
    unsigned long length, after;
    unsigned char *data_root = NULL, *data_esetroot = NULL;
    Pixmap pmap_d1, pmap_d2;

    /* local display to set closedownmode on */
    Display *disp2;
    Window root2;
    int depth2;
    int w, h;

    D(("Falling back to XSetRootWindowPixmap\n"));

    XColor color;
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));

    XAllocNamedColor(disp, cmap, "black", &color, &color);

    if (scaled) {
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
            feh_wm_set_bg_scaled(pmap_d1, im, use_filelist, 0, 0, scr->width,
                                 scr->height);
    } else if (centered) {

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
            feh_wm_set_bg_centered(pmap_d1, im, use_filelist, 0, 0, scr->width,
                                   scr->height);

        XFreeGC(disp, gc);

    } else if (filled == 1) {

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
                    feh_wm_set_bg_filled(
                        pmap_d1, im, use_filelist, xinerama_screens[i].x_org,
                        xinerama_screens[i].y_org, xinerama_screens[i].width,
                        xinerama_screens[i].height);
                }
            }
        } else
#endif /* HAVE_LIBXINERAMA */
            feh_wm_set_bg_filled(pmap_d1, im, use_filelist, 0, 0, scr->width,
                                 scr->height);

    } else if (filled == 2) {

        pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
        gcval.foreground = color.pixel;
        gc = XCreateGC(disp, root, GCForeground, &gcval);
        XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);

#ifdef HAVE_LIBXINERAMA
        if (opt.xinerama && xinerama_screens) {
            for (i = 0; i < num_xinerama_screens; i++) {
                if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
                    feh_wm_set_bg_maxed(
                        pmap_d1, im, use_filelist, xinerama_screens[i].x_org,
                        xinerama_screens[i].y_org, xinerama_screens[i].width,
                        xinerama_screens[i].height);
                }
            }
        } else
#endif /* HAVE_LIBXINERAMA */
            feh_wm_set_bg_maxed(pmap_d1, im, use_filelist, 0, 0, scr->width,
                                scr->height);

        XFreeGC(disp, gc);

    } else {
        if (use_filelist) feh_wm_load_next(&im);
        w = gib_imlib_image_get_width(im);
        h = gib_imlib_image_get_height(im);
        pmap_d1 = XCreatePixmap(disp, root, w, h, depth);
        gib_imlib_render_image_on_drawable(pmap_d1, im, 0, 0, 1, 1, 0);
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
