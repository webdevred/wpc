#include "resolution_scaling.h"

extern int scale_height(int original_width, int original_height,
                        int new_width) {
    double scale = (double)new_width / (double)original_width;
    int new_height = (int)(original_height * scale);
    return new_height;
}
