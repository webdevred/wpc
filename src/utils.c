#include "utils.h"

extern bool is_empty_string(const gchar *string_ptr) {
    return string_ptr == NULL || g_strcmp0(string_ptr, "") == 0;
}
