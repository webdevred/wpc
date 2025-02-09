/* wallpaper.h

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2020 Birte Kristina Friesel.

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

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include "config.h"
#include "feh.h"
#include "monitors.h"

#define IPC_TIMEOUT ((char *)1)
#define IPC_FAKE ((char *)2) /* Faked IPC */

#define enl_ipc_sync()                                                         \
    do {                                                                       \
        char *reply = enl_send_and_wait("nop");                                \
        if ((reply != IPC_FAKE) && (reply != IPC_TIMEOUT)) free(reply);        \
    } while (0)

extern Window ipc_win;
extern Atom ipc_atom;

extern void set_wallpapers(void);
#endif
