#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include "lightdm.h"
#include "strings.h"
#include "imagemagick.h"

#define CONFIG_FILE "/etc/lightdm/slick-greeter.conf"
#define MAX_LINE_LENGTH 1024

extern int parse_config(char ***config_ptr, int *lines_ptr);

extern void init_disp(Display **disp, Window *root);

extern void get_screen_resources(Display **disp, Window *root, XRRScreenResources **screen_resources);

extern Monitor *get_monitor(char *monitor_name);

extern Wallpaper get_wallpaper(char *wallpaper_path);
