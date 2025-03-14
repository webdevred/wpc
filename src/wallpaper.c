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

void init_x(void) {
    init_disp(&disp, &root);
    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));

    scr = ScreenOfDisplay(disp, DefaultScreen(disp));
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    return;
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
    *gc = XCreateGC(disp, root, GCForeground, &gcval);

    XPutImage(disp, pmap, *gc, ximage, 0, 0, monitor->left_x, monitor->top_y,
              monitor->width, monitor->height);

    ximage->data = NULL;
    XDestroyImage(ximage);
    free(pixels);
    pixels = NULL;

    XFreeGC(disp, *gc);

    DestroyMagickWand(wand);
    MagickWandTerminus();
    return;
}

static void wm_set_bg(Config *config) {
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
    MonitorArray *mon_arr_wrapper = list_monitors(true);
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
    wm_set_bg(config);
}
