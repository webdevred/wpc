#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wpc.h"
#include "lightdm_common.h"
#include "imagemagick.h"

extern int parse_config(char ***config_ptr, int *lines_ptr) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Error opening configuration file");
        return -1;
    }

    int lines = 0;
    char **config = NULL;
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        // Strip newline character
        line[strcspn(line, "\n")] = '\0';

        char **temp = realloc(config, (lines + 1) * sizeof(char *));
        if (temp == NULL) {
            perror("Error reallocating memory");
            fclose(file);
            for (int i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return -1;
        }
        config = temp;

        config[lines] = strdup(line);
        if (config[lines] == NULL) {
            perror("Error allocating memory for line");
            fclose(file);
            for (int i = 0; i < lines; i++) {
                free(config[i]);
            }
            free(config);
            return -1;
        }
        lines++;
    }

    fclose(file);

    *config_ptr = config;
    *lines_ptr = lines;
    return 0;
}

extern void init_disp(Display **disp, Window *root) {
    *disp = XOpenDisplay(NULL);
    if (*disp == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        exit(1);
    }

    *root = DefaultRootWindow(*disp);
}

extern void get_screen_resources(Display **disp, Window *root, XRRScreenResources **screen_resources) {
  *screen_resources = XRRGetScreenResources(*disp, *root);
  if (*screen_resources == NULL) {
    fprintf(stderr, "Unable to get screen resources\n");
    XCloseDisplay(*disp);
    exit(1);
  }
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

        if (outputInfo->connection == RR_Connected &&
            strcmp(monitor_name, outputInfo->name) == 0) {
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

extern Wallpaper get_wallpaper(gchar *wallpaper_path) {
    Wallpaper wallpaper;

    wallpaper.path = wallpaper_path;

    set_resolution(&wallpaper);

    return wallpaper;
}
