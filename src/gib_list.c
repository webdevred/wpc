/* gib_list.c

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

#include "gib_list.h"
#include "debug.h"
#include "utils.h"
#include <time.h>

gib_list *gib_list_new(void) {
    gib_list *l;

    l = (gib_list *)emalloc(sizeof(gib_list));
    l->data = NULL;
    l->next = NULL;
    l->prev = NULL;
    return (l);
}

extern gib_list *gib_list_add_front(gib_list *root, void *data) {
    gib_list *l;

    l = gib_list_new();
    l->next = root;
    l->data = data;
    if (root) root->prev = l;
    return (l);
}

int gib_list_length(gib_list *l) {
    int length;

    length = 0;
    while (l) {
        length++;
        l = l->next;
    }
    return (length);
}
