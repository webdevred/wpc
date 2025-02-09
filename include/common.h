#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#define MAX_LINE_LENGTH 1024
#define CONFIG_FILE "/etc/lightdm/slick-greeter.conf"

extern int lightdm_parse_config(char ***config_ptr, int *lines_ptr);

#ifdef DM_CONFIG_PAYLOAD
typedef struct {
    char *config_file;
    char *tmp_file_path;
    char *dst_file_path;
    bool primary_monitor;
} DmConfigPayload;
#endif
