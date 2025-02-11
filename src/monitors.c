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

extern Monitor *wm_list_monitors(int *number_of_monitors) {
    Display *display;
    Window root;
    XRRScreenResources *screen_resources;

    init_disp(&display, &root);

    get_screen_resources(&display, &root, &screen_resources);

    XRROutputInfo *outputInfo;
    XRRCrtcInfo *crtcInfo;

    RROutput primaryOutput;

    primaryOutput = XRRGetOutputPrimary(display, root);

    *number_of_monitors = 0;
    Monitor *monitors = NULL;

    for (int i = 0; i < screen_resources->noutput; i++) {
        outputInfo = XRRGetOutputInfo(display, screen_resources,
                                      screen_resources->outputs[i]);

        if (outputInfo->connection == RR_Connected) {
            crtcInfo =
                XRRGetCrtcInfo(display, screen_resources, outputInfo->crtc);
            if (crtcInfo) {
                (*number_of_monitors)++;
                monitors =
                    realloc(monitors, *number_of_monitors * sizeof(Monitor));
                monitors[i].name =
                    malloc((strlen(outputInfo->name) + 1) * sizeof(char));
                strcpy(monitors[i].name, outputInfo->name);
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

    return monitors;
}
#ifdef WPC_ENABLE_HELPER
extern void dm_list_monitors(Monitor *primary_monitor,
                             Monitor *secondary_monitor,
                             int *number_of_other_monitors) {
    Display *display;
    Window root;
    XRRScreenResources *screen_resources;

    init_disp(&display, &root);
    get_screen_resources(&display, &root, &screen_resources);

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
        XRRFreeScreenResources(screen_resources);
        XCloseDisplay(display);
        exit(1);
    }

    for (int i = 0; i < screen_resources->noutput; i++) {
        outputInfo = XRRGetOutputInfo(display, screen_resources,
                                      screen_resources->outputs[i]);

        if (outputInfo->connection == RR_Connected) {
            crtcInfo =
                XRRGetCrtcInfo(display, screen_resources, outputInfo->crtc);
            if (crtcInfo) {
                unsigned int crtc_width = crtcInfo->width;
                unsigned int crtc_height = crtcInfo->height;

                if (screen_resources->outputs[i] == primaryOutput) {
                    primary_monitor->name =
                        malloc((strlen(outputInfo->name) + 1) * sizeof(char));
                    strcpy(primary_monitor->name, outputInfo->name);

                    primary_monitor->width = crtc_width;
                    primary_monitor->height = crtc_height;
                    primary_monitor->primary = true;
                } else {
                    if (secondary_monitor->width < crtc_width ||
                        secondary_monitor->height < crtc_height) {
                        secondary_monitor->name = malloc(
                            (strlen(outputInfo->name) + 1) * sizeof(char));
                        strcpy(secondary_monitor->name, outputInfo->name);
                        secondary_monitor->width = crtc_width;
                        secondary_monitor->height = crtc_height;
                    }
                    (*number_of_other_monitors)++;
                }
            }
            XRRFreeCrtcInfo(crtcInfo);
        }

        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screen_resources);
    XCloseDisplay(display);
}
#endif
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
                monitor->name =
                    malloc((strlen(monitor_name) + 1) * sizeof(char));
                strcpy(monitor->name, monitor_name);
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
