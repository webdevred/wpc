/* gib_imlib.c

Copyright (C) 1999,2000 Tom Gilbert.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "gib_imlib.h"
#include "debug.h"
#include "utils.h"

int gib_imlib_image_get_width(Imlib_Image im) {
    imlib_context_set_image(im);
    return imlib_image_get_width();
}

int gib_imlib_image_get_height(Imlib_Image im) {
    imlib_context_set_image(im);
    return imlib_image_get_height();
}

int gib_imlib_image_has_alpha(Imlib_Image im) {
    imlib_context_set_image(im);
    return imlib_image_has_alpha();
}

void gib_imlib_free_image_and_decache(Imlib_Image im) {
    imlib_context_set_image(im);
    imlib_free_image_and_decache();
}

void gib_imlib_render_image_on_drawable(Drawable d, Imlib_Image im, int x,
                                        int y, char dither, char blend,
                                        char alias) {
    imlib_context_set_image(im);
    imlib_context_set_drawable(d);
    imlib_context_set_anti_alias(alias);
    imlib_context_set_dither(dither);
    imlib_context_set_blend(blend);
    imlib_context_set_angle(0);
    imlib_render_image_on_drawable(x, y);
}

void gib_imlib_render_image_on_drawable_at_size(Drawable d, Imlib_Image im,
                                                int x, int y, int w, int h,
                                                char dither, char blend,
                                                char alias) {
    imlib_context_set_image(im);
    imlib_context_set_drawable(d);
    imlib_context_set_anti_alias(alias);
    imlib_context_set_dither(dither);
    imlib_context_set_blend(blend);
    imlib_context_set_angle(0);
    imlib_render_image_on_drawable_at_size(x, y, w, h);
}

void gib_imlib_render_image_part_on_drawable_at_size(
    Drawable d, Imlib_Image im, int sx, int sy, int sw, int sh, int dx, int dy,
    int dw, int dh, char dither, char blend, char alias) {
    imlib_context_set_image(im);
    imlib_context_set_drawable(d);
    imlib_context_set_anti_alias(alias);
    imlib_context_set_dither(dither);
    imlib_context_set_blend(blend);
    imlib_context_set_angle(0);
    imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
} 

char *gib_imlib_image_format(Imlib_Image im) {
    imlib_context_set_image(im);
    return imlib_image_format();
}

void gib_imlib_save_image(Imlib_Image im, char *file) {
    char *tmp;

    imlib_context_set_image(im);
    tmp = strrchr(file, '.');
    if (tmp) {
        char *p, *pp;
        p = strdup(tmp + 1);
        pp = p;
        while (*pp) {
            *pp = tolower(*pp);
            pp++;
        }
        imlib_image_set_format(p);
        free(p);
    }
    imlib_save_image(file);
}

