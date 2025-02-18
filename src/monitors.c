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
#include "structs.h"

extern void init_disp(Display **disp, Window *root) {
    *disp = XOpenDisplay(NULL);
    if (*disp == NULL) {
        fprintf(stderr, "Unable to open X display\n");
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
    int amount_used = 0;

    Display *display;
    Window root;

    init_disp(&display, &root);

    XRRMonitorInfo *x_monitors;
    x_monitors = XRRGetMonitors(display, root, 0, &amount_used);

    for (int i = 0; i < amount_used; i++) {
        monitors[i].name = XGetAtomName(display, x_monitors[i].name);
        monitors[i].width = x_monitors[i].width;
        monitors[i].height = x_monitors[i].height;
        monitors[i].horizontal_position = x_monitors[i].x;
        monitors[i].vertical_position = x_monitors[i].y;
        monitors[i].primary = x_monitors[i].primary;
    }

    array_wrapper->amount_allocated = (ushort)amount_used;
    array_wrapper->amount_used = (ushort)amount_used;

    return array_wrapper;
}

extern Monitor *get_monitor(const char *monitor_name) {
    Display *display;
    Window root;

    init_disp(&display, &root);

    Monitor *monitor = malloc(sizeof(Monitor));

    XRRMonitorInfo *x_monitors;
    int amount_used = 0;
    x_monitors = XRRGetMonitors(display, root, 0, &amount_used);

    bool found = false;
    for (int i = 0; i < amount_used; i++) {
        char *name = XGetAtomName(display, x_monitors[i].name);
        if (strcmp(name, monitor_name) == 0) {
            found = true;
            monitor->name = strdup(monitor_name);
            monitor->width = x_monitors[i].width;
            monitor->height = x_monitors[i].height;
            monitor->horizontal_position = x_monitors[i].x;
            monitor->vertical_position = x_monitors[i].y;
            monitor->primary = x_monitors[i].primary;
        }
    }

    if (found == false) {
        free(monitor);
        return NULL;
    }

    return monitor;
}
