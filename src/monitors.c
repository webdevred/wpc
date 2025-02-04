#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

extern Monitor *wm_list_monitors(int *number_of_monitors) {
    Display *display;
    Window root;
    XRRScreenResources *screenResources;

    init_disp(&display, &root, &screenResources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    *number_of_monitors = screenResources->noutput;

    Monitor *monitors =
        (Monitor *)malloc(screenResources->noutput * sizeof(Monitor));
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
                monitors[i].name =
                    malloc((strlen(outputInfo->name) + 1) * sizeof(char));
                monitors[i].name = outputInfo->name;
                monitors[i].width = crtcInfo->width;
                monitors[i].height = crtcInfo->height;
                monitors[i].primary =
                    (screenResources->outputs[i] == primaryOutput);

                XRRFreeCrtcInfo(crtcInfo);
            }
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screenResources);
    XCloseDisplay(display);

    return monitors;
}

extern void dm_list_monitors(Monitor *primary_monitor,
                             Monitor *secondary_monitor,
                             int *number_of_other_monitors) {
    Display *display;
    Window root;
    XRRScreenResources *screenResources;

    init_disp(&display, &root, &screenResources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    secondary_monitor->primary = false;
    secondary_monitor->width = 0;
    secondary_monitor->height = 0;
    *number_of_other_monitors = 0;

    if (primary_monitor == NULL) {
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
                primary_monitor->name =
                    malloc((strlen(outputInfo->name) + 1) * sizeof(char));
                strcpy(primary_monitor->name, outputInfo->name);

                primary_monitor->width = crtcInfo->width;
                primary_monitor->height = crtcInfo->height;
                primary_monitor->primary = true;
            } else {
                unsigned int crtc_width = crtcInfo->width;
                unsigned int crtc_height = crtcInfo->width;
                if (secondary_monitor->width < crtc_width ||
                    secondary_monitor->height < crtc_height) {
                    secondary_monitor->name =
                        malloc((strlen(outputInfo->name) + 1) * sizeof(char));
                    strcpy(secondary_monitor->name, outputInfo->name);
                    secondary_monitor->width = crtc_width;
                    secondary_monitor->height = crtc_height;
                }
                (*number_of_other_monitors)++;
            }
            XRRFreeCrtcInfo(crtcInfo);
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screenResources);
    XCloseDisplay(display);
}

extern Monitor *get_monitor(char *monitor_name) {
    Display *display;
    Window root;
    XRRScreenResources *screenResources;

    init_disp(&display, &root, &screenResources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    Monitor *monitor = malloc(sizeof(Monitor));
    if (monitor == NULL) {
        fprintf(stderr, "Failed to allocate memory for monitors\n");
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        exit(1);
    }

    for (int i = 0; i < screenResources->noutput; i++) {
        outputInfo = XRRGetOutputInfo(display, screenResources,
                                      screenResources->outputs[i]);

        if (outputInfo->connection == RR_Connected &&
            strcmp(monitor_name, outputInfo->name)) {
            crtcInfo =
                XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
            if (crtcInfo) {
                monitor->name =
                    malloc((strlen(monitor_name) + 1) * sizeof(char));
                strcpy(monitor->name, monitor_name);
                monitor->width = crtcInfo->width;
                monitor->height = crtcInfo->height;
                monitor->primary =
                    (screenResources->outputs[i] == primaryOutput);

                XRRFreeCrtcInfo(crtcInfo);
            }
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screenResources);
    XCloseDisplay(display);

    return monitor;
}
