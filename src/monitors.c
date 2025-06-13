#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <glib.h>

#include "monitors.h"

Display *querying_display = NULL;
Window querying_root = 0;
int querying_depth;

Display *rendering_display = NULL;
Window rendering_root = 0;
Visual *rendering_visual = NULL;
int rendering_depth;
Colormap rendering_colormap;
Screen *rendering_screen;

static XRRScreenResources *get_screen_resources() {
    XRRScreenResources *screen_resources =
        XRRGetScreenResources(querying_display, querying_root);
    if (screen_resources == NULL) {
        g_error("Unable to get screen resources\n");
        XCloseDisplay(querying_display);
        exit(1);
    }
    return screen_resources;
}

static Display *open_display() {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        g_error("Unable to open X display\n");
        exit(1);
    }
    return display;
}

extern void init_x11(void) {
    querying_display = open_display();
    querying_root = DefaultRootWindow(querying_display);
    querying_depth =
        DefaultDepth(querying_display, DefaultScreen(querying_display));

    rendering_display = open_display();
    rendering_root = DefaultRootWindow(rendering_display);

    rendering_visual =
        DefaultVisual(rendering_display, DefaultScreen(rendering_display));
    rendering_depth =
        DefaultDepth(rendering_display, DefaultScreen(rendering_display));
    rendering_colormap =
        DefaultColormap(rendering_display, DefaultScreen(rendering_display));
    rendering_screen =
        ScreenOfDisplay(rendering_display, DefaultScreen(rendering_display));
}

extern void free_monitors(MonitorArray *arr) {
    Monitor *monitors = (Monitor *)arr->data;
    for (unsigned int i = 0; i < arr->amount_used; i++) {
        free(monitors[i].name);
    }
    free(monitors);
    free(arr);

    XCloseDisplay(querying_display);
    XCloseDisplay(rendering_display);
}

extern MonitorArray *list_monitors(const bool virtual_monitors) {
    MonitorArray *array_wrapper = malloc(sizeof(MonitorArray));
    if (!array_wrapper) {
        return NULL;
    }

    if (virtual_monitors) {
        int amount_used = 0;
        XRRMonitorInfo *x_monitors;
        x_monitors =
            XRRGetMonitors(querying_display, querying_root, 0, &amount_used);

        array_wrapper->data = malloc(amount_used * sizeof(Monitor));
        if (!array_wrapper->data) {
            free(array_wrapper);
            return NULL;
        }

        Monitor *monitors = (Monitor *)array_wrapper->data;

        for (int i = 0; i < amount_used; i++) {
            monitors[i].name =
                XGetAtomName(querying_display, x_monitors[i].name);
            monitors[i].width = x_monitors[i].width;
            monitors[i].height = x_monitors[i].height;
            monitors[i].left_x = x_monitors[i].x;
            monitors[i].top_y = x_monitors[i].y;
            monitors[i].primary = x_monitors[i].primary;

            monitors[i].belongs_to_config = FALSE;
            monitors[i].config_id = 0;
            monitors[i].wallpaper = NULL;
        }

        XRRFreeMonitors(x_monitors);

        array_wrapper->amount_allocated = (gushort)amount_used;
        array_wrapper->amount_used = (gushort)amount_used;
    } else {
        array_wrapper->data = NULL;
        Monitor *monitors = (Monitor *)array_wrapper->data;
        gushort amount_used = 0;
        gushort amount_allocated = 0;
        XRRScreenResources *screen_resources = get_screen_resources();

        XRROutputInfo *outputInfo;
        XRRCrtcInfo *crtcInfo;

        RROutput primaryOutput;

        primaryOutput = XRRGetOutputPrimary(querying_display, querying_root);

        for (int i = 0; i < screen_resources->noutput; i++) {
            outputInfo = XRRGetOutputInfo(querying_display, screen_resources,
                                          screen_resources->outputs[i]);

            if (outputInfo->connection == RR_Connected) {
                crtcInfo = XRRGetCrtcInfo(querying_display, screen_resources,
                                          outputInfo->crtc);
                if (crtcInfo->mode != None) {
                    amount_used++;
                    if (amount_used >= amount_allocated) {
                        amount_allocated += 3;
                        array_wrapper->data = realloc(
                            monitors, amount_allocated * sizeof(Monitor));
                        monitors = (Monitor *)array_wrapper->data;
                    }
                    monitors[i].name = strdup(outputInfo->name);
                    monitors[i].width = crtcInfo->width;
                    monitors[i].height = crtcInfo->height;
                    monitors[i].left_x = crtcInfo->x;
                    monitors[i].top_y = crtcInfo->y;
                    monitors[i].primary =
                        (screen_resources->outputs[i] == primaryOutput);

                    XRRFreeCrtcInfo(crtcInfo);

                    monitors[i].wallpaper = NULL;
                    monitors[i].belongs_to_config = FALSE;
                    monitors[i].config_id = 0;
                }
            }

            XRRFreeOutputInfo(outputInfo);
        }

        XRRFreeScreenResources(screen_resources);

        array_wrapper->amount_allocated = amount_allocated;
        array_wrapper->amount_used = amount_used;
    }

    return array_wrapper;
}
