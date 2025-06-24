#include "common.h"
#include <assert.h>
#include <cjson/cJSON.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <libgen.h>
#include <string.h>

extern int create_parent_dirs(const char *file_path, mode_t mode) {
    assert(file_path && *file_path);
    char *path_copy = strdup(file_path);
    if (!path_copy) return -1;
    for (char *p = strchr(path_copy + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(path_copy, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    free(path_copy);
    return 0;
}
