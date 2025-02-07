#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#define CONFIG_FILE "/etc/lightdm/slick-greeter.conf"
#define MAX_LINE_LENGTH 1024

extern int parse_config(char ***config_ptr, int *lines_ptr);
