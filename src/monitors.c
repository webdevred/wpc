#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "monitors.h"
#include "strings.h"
#include "structs.h"

extern void init_disp(Display **disp, Window *root) {
    *disp = XOpenDisplay(NULL);
    if (*disp == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        exit(1);
    }

    *root = DefaultRootWindow(*disp);
}

extern void get_screen_resources(Display **disp, Window *root,
                                 XRRScreenResources **screen_resources) {
    *screen_resources = XRRGetScreenResources(*disp, *root);
    if (*screen_resources == NULL) {
        fprintf(stderr, "Unable to get screen resources\n");
        XCloseDisplay(*disp);
        exit(1);
    }
}

extern void free_monitors(ArrayWrapper *arr) {
    Monitor *monitors = (Monitor *)arr->data;
    for (int i = 0; i < arr->amount_used; i++) {
        free(monitors[i].name);
    }
    free(monitors);
    free(arr);
}

extern ArrayWrapper *list_monitors() {
    ArrayWrapper *array_wrapper = malloc(sizeof(ArrayWrapper));
    if (!array_wrapper) {
        return NULL;
    }

    array_wrapper->data = malloc(3 * sizeof(Monitor));
    if (!array_wrapper->data) {
        free(array_wrapper);
        return NULL;
    }

    Monitor *monitors = (Monitor *)array_wrapper->data;
    ushort amount_used = 0;
    ushort amount_allocated = 3;

    Display *display;
    Window root;
    XRRScreenResources *screen_resources;

    init_disp(&display, &root);

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
                    monitors =
                        realloc(monitors, amount_allocated * sizeof(Monitor));
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

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screen_resources);
    XCloseDisplay(display);

    array_wrapper->amount_allocated = amount_allocated;
    array_wrapper->amount_used = amount_used;

    return array_wrapper;
}

extern Monitor *get_monitor(char *monitor_name) {
    Display *display;
    Window root;
    XRRScreenResources *screen_resources;

    init_disp(&display, &root);
    get_screen_resources(&display, &root, &screen_resources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    Monitor *monitor = malloc(sizeof(Monitor));
    if (monitor == NULL) {
        fprintf(stderr, "Failed to allocate memory for monitors\n");
        XRRFreeScreenResources(screen_resources);
        XCloseDisplay(display);
        exit(1);
    }

    for (int i = 0; i < screen_resources->noutput; i++) {
        outputInfo = XRRGetOutputInfo(display, screen_resources,
                                      screen_resources->outputs[i]);
        if (strcmp(monitor_name, outputInfo->name) == 0) {
            crtcInfo =
                XRRGetCrtcInfo(display, screen_resources, outputInfo->crtc);
            if (crtcInfo) {
                monitor->name = strdup(monitor_name);
                monitor->width = crtcInfo->width;
                monitor->height = crtcInfo->height;
                monitor->horizontal_position = crtcInfo->x;
                monitor->vertical_position = crtcInfo->y;
                monitor->primary =
                    (screen_resources->outputs[i] == primaryOutput);

                XRRFreeCrtcInfo(crtcInfo);
            }
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screen_resources);
    XCloseDisplay(display);

    return monitor;
}
