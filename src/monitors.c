#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <glib.h>

#include "monitors.h"
#include "structs.h"

static void get_screen_resources(Display **disp, Window *root,
                                 XRRScreenResources **screen_resources) {
    *screen_resources = XRRGetScreenResources(*disp, *root);
    if (*screen_resources == NULL) {
        g_error("Unable to get screen resources\n");
        XCloseDisplay(*disp);
        exit(1);
    }
}

extern void init_disp(Display **disp, Window *root) {
    *disp = XOpenDisplay(NULL);
    if (*disp == NULL) {
        g_error("Unable to open X display\n");
        exit(1);
    }

    *root = DefaultRootWindow(*disp);
}

extern void free_monitors(ArrayWrapper *arr) {
    Monitor *monitors = (Monitor *)arr->data;
    for (unsigned int i = 0; i < arr->amount_used; i++) {
        free(monitors[i].name);
    }
    free(monitors);
    free(arr);
}

extern ArrayWrapper *list_monitors(const bool virtual_monitors) {
    ArrayWrapper *array_wrapper = malloc(sizeof(ArrayWrapper));
    if (!array_wrapper) {
        return NULL;
    }

    Display *display;
    Window root;
    init_disp(&display, &root);

    if (virtual_monitors) {
        int amount_used = 0;
        XRRMonitorInfo *x_monitors;
        x_monitors = XRRGetMonitors(display, root, 0, &amount_used);

        array_wrapper->data = malloc(amount_used * sizeof(Monitor));
        if (!array_wrapper->data) {
            free(array_wrapper);
            return NULL;
        }

        Monitor *monitors = (Monitor *)array_wrapper->data;

        for (int i = 0; i < amount_used; i++) {
            monitors[i].name = XGetAtomName(display, x_monitors[i].name);
            monitors[i].width = x_monitors[i].width;
            monitors[i].height = x_monitors[i].height;
            monitors[i].horizontal_position = x_monitors[i].x;
            monitors[i].vertical_position = x_monitors[i].y;
            monitors[i].primary = x_monitors[i].primary;
        }

        array_wrapper->amount_allocated = (gushort)amount_used;
        array_wrapper->amount_used = (gushort)amount_used;
    } else {
        Monitor *monitors = (Monitor *)array_wrapper->data;
        gushort amount_used = 0;
        gushort amount_allocated = 0;
        XRRScreenResources *screen_resources;

        get_screen_resources(&display, &root, &screen_resources);

        XRROutputInfo *outputInfo;
        XRRCrtcInfo *crtcInfo;

        RROutput primaryOutput;

        primaryOutput = XRRGetOutputPrimary(display, root);

        for (int i = 0; i < screen_resources->noutput; i++) {
            outputInfo = XRRGetOutputInfo(display, screen_resources,
                                          screen_resources->outputs[i]);

            if (outputInfo->connection == RR_Connected) {
                crtcInfo =
                    XRRGetCrtcInfo(display, screen_resources, outputInfo->crtc);
                if (crtcInfo->mode != None) {
                    amount_used++;
                    if (amount_used >= amount_allocated) {
                        amount_allocated += 3;
                        monitors = realloc(monitors,
                                           amount_allocated * sizeof(Monitor));
                    }
                    monitors[i].name = strdup(outputInfo->name);
                    monitors[i].width = crtcInfo->width;
                    monitors[i].height = crtcInfo->height;
                    monitors[i].horizontal_position = crtcInfo->x;
                    monitors[i].vertical_position = crtcInfo->y;
                    monitors[i].primary =
                        (screen_resources->outputs[i] == primaryOutput);

                    XRRFreeCrtcInfo(crtcInfo);
                }
            }
        }

        array_wrapper->amount_allocated = amount_allocated;
        array_wrapper->amount_used = amount_used;
    }

    return array_wrapper;
}
