#include "glib.h"
#include "monitors.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wand/MagickWand.h>
#include <wand/magick-image.h>

#include "wallpaper.h"
#include "wallpaper_transformation.h"

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

extern void init_x(void) {
    init_disp(&disp, &root);
    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));

    scr = ScreenOfDisplay(disp, DefaultScreen(disp));
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    return;
}

static void set_bg_for_monitor(const gchar *wallpaper_path,
                               gchar *conf_bg_fb_color, BgMode bg_mode,
                               Monitor *monitor, Pixmap pmap) {
    MagickWandGenesis();

    MagickWand *wand = NewMagickWand();

    if (MagickReadImage(wand, wallpaper_path) == MagickFalse) {
        DestroyMagickWand(wand);
        MagickWandTerminus();
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
    XImage *ximage = XCreateImage(disp, vis, depth, ZPixmap, 0, (char *)pixels,
                                  monitor->width, monitor->height, 32, 0);

    XGCValues gcval;
    gcval.foreground = None;
    GC gc = XCreateGC(disp, root, GCForeground, &gcval);

    XPutImage(disp, pmap, gc, ximage, 0, 0, monitor->left_x, monitor->top_y,
              monitor->width, monitor->height);

    ximage->data = NULL;
    XDestroyImage(ximage);
    free(pixels);
    pixels = NULL;

    XFreeGC(disp, gc);

    DestroyMagickWand(wand);
    MagickWandTerminus();
    return;
}

static Atom get_atom(Display *display, char *atom_name, Bool only_if_exists) {
    Atom atom = XInternAtom(display, atom_name, only_if_exists);
    if (atom == None) {
        g_error("failed to get atom %s", atom_name);
    }
    return atom;
}

extern void set_wallpapers(Config *config, MonitorArray *mon_arr_wrapper) {
    GC gc;

    Atom prop_root, prop_esetroot;
    Pixmap pmap_d1, pmap_d2;

    Display *disp2;
    Window root2;
    gushort depth2;

    pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
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

        set_bg_for_monitor(wallpaper_path, bg_fallback_color, bg_mode, monitor,
                           pmap_d1);
    }

    /* create new display, copy pixmap to new display */
    disp2 = XOpenDisplay(NULL);
    if (!disp2) g_error("Can't reopen X display.");
    root2 = RootWindow(disp2, DefaultScreen(disp2));
    depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
    XSync(disp, False);
    pmap_d2 = XCreatePixmap(disp2, root2, scr->width, scr->height, depth2);
    XGCValues gcvalues = {
        .fill_style = FillTiled,
        .tile = pmap_d1,
    };
    gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(disp2, pmap_d2, gc, 0, 0, scr->width, scr->height);

    prop_root = get_atom(disp2, "_XROOTPMAP_ID", False);
    prop_esetroot = get_atom(disp2, "ESETROOT_PMAP_ID", False);

    if (prop_root == None || prop_esetroot == None) {
        g_critical("creation of pixmap property failed.");
        goto cleanup;
    }

    XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&pmap_d2, 1);
    XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *)&pmap_d2, 1);

    XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
    XClearWindow(disp2, root2);
    XFlush(disp2);
    XSetCloseDownMode(disp2, RetainPermanent);

cleanup:
    XFreeGC(disp2, gc);
    XSync(disp2, False);
    XSync(disp, False);
    XFreePixmap(disp, pmap_d1);

    XCloseDisplay(disp2);

    return;
}
