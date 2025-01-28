#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "monitors.h"

void init_disp(Display **display, Window *root,
               XRRScreenResources **screen_resources) {
    *display = XOpenDisplay(NULL);
    if (*display == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        exit(1);
    }

    *root = DefaultRootWindow(*display);

    *screen_resources = XRRGetScreenResources(*display, *root);
    if (*screen_resources == NULL) {
        fprintf(stderr, "Unable to get screen resources\n");
        XCloseDisplay(*display);
        exit(1);
    }
}

extern void wm_list_monitors(Monitor **monitors, int *number_of_monitors) {
    Display *display;
    Window root;
    XRRScreenResources *screenResources;

    init_disp(&display, &root, &screenResources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    *number_of_monitors = screenResources->noutput;

    *monitors = (Monitor *)malloc(screenResources->noutput * sizeof(Monitor));
    if (monitors == NULL) {
        fprintf(stderr, "Failed to allocate memory for monitors\n");
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        exit(1);
    }

    for (int i = 0; i < screenResources->noutput; i++) {
        outputInfo = XRRGetOutputInfo(display, screenResources,
                                      screenResources->outputs[i]);

        if (outputInfo->connection == RR_Connected) {
            crtcInfo =
                XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
            if (crtcInfo) {
                (*monitors)[i].width = crtcInfo->width;
                (*monitors)[i].height = crtcInfo->height;
                (*monitors)[i].primary =
                    (screenResources->outputs[i] == primaryOutput);

                XRRFreeCrtcInfo(crtcInfo);
            }
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screenResources);
    XCloseDisplay(display);
}

extern void dm_list_monitors(Monitor **primary_monitor,
                             Monitor **other_monitors,
                             int *number_of_other_monitors) {
    Display *display;
    Window root;
    XRRScreenResources *screenResources;

    init_disp(&display, &root, &screenResources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    *primary_monitor = malloc(sizeof(Monitor));
    if (*primary_monitor == NULL) {
        fprintf(stderr, "Failed to allocate memory for monitors\n");
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        exit(1);
    }

    for (int i = 0; i < screenResources->noutput; i++) {
        outputInfo = XRRGetOutputInfo(display, screenResources,
                                      screenResources->outputs[i]);

        if (outputInfo->connection == RR_Connected) {
            crtcInfo =
                XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
            if (crtcInfo && screenResources->outputs[i] == primaryOutput) {
                (*primary_monitor)->width = crtcInfo->width;
                (*primary_monitor)->height = crtcInfo->height;
                (*primary_monitor)->primary = true;

                XRRFreeCrtcInfo(crtcInfo);
            } else {
                other_monitors =
                    realloc(other_monitors,
                            sizeof(Monitor) * (*number_of_other_monitors + 1));
                other_monitors[*number_of_other_monitors]->width =
                    crtcInfo->width;
                other_monitors[*number_of_other_monitors]->height =
                    crtcInfo->height;
                other_monitors[*number_of_other_monitors]->primary = false;
                (*number_of_other_monitors)++;
            }
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screenResources);
    XCloseDisplay(display);
}
