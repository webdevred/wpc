/* imlib.c

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

#include "feh.h"
#include "filelist.h"
#include "signals.h"
#include "winwidget.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef HAVE_LIBEXIF
#include "exif.h"
#endif

#ifdef HAVE_LIBMAGIC
#include <magic.h>

magic_t magic = NULL;
#endif

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
XContext xid_context = 0;
Window root = 0;

/* Xinerama support */
#ifdef HAVE_LIBXINERAMA
XineramaScreenInfo *xinerama_screens = NULL;
int xinerama_screen;
int num_xinerama_screens;
#endif /* HAVE_LIBXINERAMA */

int childpid = 0;

#ifdef HAVE_LIBXINERAMA
void init_xinerama(void) {
    if (opt.xinerama && XineramaIsActive(disp)) {
        int major, minor, px, py, i;

        /* discarded */
        Window dw;
        int di;
        unsigned int du;

        XineramaQueryVersion(disp, &major, &minor);
        xinerama_screens = XineramaQueryScreens(disp, &num_xinerama_screens);

        if (opt.xinerama_index >= 0)
            xinerama_screen = opt.xinerama_index;
        else {
            xinerama_screen = 0;
            XQueryPointer(disp, root, &dw, &dw, &px, &py, &di, &di, &du);
            for (i = 0; i < num_xinerama_screens; i++) {
                if (XY_IN_RECT(px, py, xinerama_screens[i].x_org,
                               xinerama_screens[i].y_org,
                               xinerama_screens[i].width,
                               xinerama_screens[i].height)) {
                    xinerama_screen = i;
                    break;
                }
            }
        }
    }
}
#endif /* HAVE_LIBXINERAMA */

void init_x_and_imlib(void) {
    disp = XOpenDisplay(NULL);
    if (!disp) eprintf("Can't open X display. It *is* running, yeah?");
    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));
    root = RootWindow(disp, DefaultScreen(disp));
    scr = ScreenOfDisplay(disp, DefaultScreen(disp));
    xid_context = XUniqueContext();

#ifdef HAVE_LIBXINERAMA
    init_xinerama();
#endif /* HAVE_LIBXINERAMA */

    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_color_modifier(NULL);
    imlib_context_set_progress_function(NULL);
    imlib_context_set_operation(IMLIB_OP_COPY);
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    imlib_set_cache_size(1024 * 1024);

    return;
}

void feh_print_load_error(char *file, winwidget w, Imlib_Load_Error err,
                          enum feh_load_error feh_err) {
    if (err == IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS)
        eprintf("%s - Out of file descriptors while loading", file);
    else {
        switch (feh_err) {
        case LOAD_ERROR_IMLIB:
            // handled in the next switch/case statement
            break;
        case LOAD_ERROR_IMAGEMAGICK:
            im_weprintf(w, "%s - No ImageMagick loader for that file format",
                        file);
            break;
        case LOAD_ERROR_CURL:
            im_weprintf(w, "%s - libcurl was unable to retrieve the file",
                        file);
            break;
        case LOAD_ERROR_DCRAW:
            im_weprintf(w, "%s - Unable to open preview via dcraw", file);
            break;
        case LOAD_ERROR_MAGICBYTES:
            im_weprintf(
                w, "%s - Does not look like an image (magic bytes missing)",
                file);
            break;
        }
        if (feh_err != LOAD_ERROR_IMLIB) {
            return;
        }
        switch (err) {
        case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
            im_weprintf(w, "%s - File does not exist", file);
            break;
        case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
            im_weprintf(w, "%s - Directory specified for image filename", file);
            break;
        case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
            im_weprintf(w, "%s - No read access", file);
            break;
        case IMLIB_LOAD_ERROR_UNKNOWN:
        case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
            im_weprintf(w, "%s - No Imlib2 loader for that file format", file);
            break;
        case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
            im_weprintf(w, "%s - Path specified is too long", file);
            break;
        case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
            im_weprintf(w, "%s - Path component does not exist", file);
            break;
        case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
            im_weprintf(w, "%s - Path component is not a directory", file);
            break;
        case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
            im_weprintf(w, "%s - Path points outside address space", file);
            break;
        case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
            im_weprintf(w, "%s - Too many levels of symbolic links", file);
            break;
        case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
            im_weprintf(w, "While loading %s - Out of memory", file);
            break;
        case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
            im_weprintf(w, "%s - Cannot write to directory", file);
            break;
        case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
            im_weprintf(w, "%s - Cannot write - out of disk space", file);
            break;
#if defined(IMLIB2_VERSION_MAJOR) && defined(IMLIB2_VERSION_MINOR) &&          \
    (IMLIB2_VERSION_MAJOR > 1 || IMLIB2_VERSION_MINOR > 7)
        case IMLIB_LOAD_ERROR_IMAGE_READ:
            im_weprintf(w, "%s - Invalid image file", file);
            break;
        case IMLIB_LOAD_ERROR_IMAGE_FRAME:
            im_weprintf(w, "%s - Requested frame not in image", file);
            break;
#endif
        default:
            im_weprintf(w, "While loading %s - Unknown error (%d)", file, err);
            break;
        }
    }
}

#ifdef HAVE_LIBMAGIC
void uninit_magic(void) {
    if (!magic) {
        return;
    }

    magic_close(magic);
    magic = NULL;
}
void init_magic(void) {
    if (getenv("FEH_SKIP_MAGIC")) {
        return;
    }

    if (!(magic = magic_open(MAGIC_NONE))) {
        weprintf("unable to initialize magic library\n");
        return;
    }

    if (magic_load(magic, NULL) != 0) {
        weprintf("cannot load magic database: %s\n", magic_error(magic));
        uninit_magic();
    }
}

/*
 * This is a workaround for an Imlib2 regression, causing unloadable image
 * detection to be excessively slow (and, thus, causing feh to hang for a while
 * when encountering an unloadable image). We use magic byte detection to
 * avoid calling Imlib2 for files it probably cannot handle. See
 * <https://phab.enlightenment.org/T8739> and
 * <https://github.com/derf/feh/issues/505>.
 */
int feh_is_image(feh_file *file, int magic_flags) {
    const char *mime_type = NULL;

    if (!magic) {
        return 1;
    }

    magic_setflags(magic, MAGIC_MIME_TYPE | MAGIC_SYMLINK | magic_flags);
    mime_type = magic_file(magic, file->filename);

    if (!mime_type) {
        return 0;
    }

    D(("file %s has mime type: %s\n", file->filename, mime_type));

    if (strncmp(mime_type, "image/", 6) == 0) {
        return 1;
    }

    /* no infinite loop on compressed content, please */
    if (magic_flags) {
        return 0;
    }

    /* imlib2 supports loading compressed images, let's have a look inside */
    if (strcmp(mime_type, "application/gzip") == 0 ||
        strcmp(mime_type, "application/x-bzip2") == 0 ||
        strcmp(mime_type, "application/x-xz") == 0) {
        return feh_is_image(file, MAGIC_COMPRESS);
    }

    return 0;
}
#else
int feh_is_image(__attribute__((unused)) feh_file *file,
                 __attribute__((unused)) int magic_flags) {
    return 1;
}
#endif

extern int feh_load_image(Imlib_Image *im, feh_file *file) {
    Imlib_Load_Error err = IMLIB_LOAD_ERROR_NONE;
    enum feh_load_error feh_err = LOAD_ERROR_IMLIB;
    enum {
        SRC_IMLIB,
        SRC_HTTP,
        SRC_MAGICK,
        SRC_DCRAW
    } image_source = SRC_IMLIB;
    char *tmpname = NULL;
    char *real_filename = NULL;

    D(("filename is %s, image is %p\n", file->filename, im));

    if (!file || !file->filename) return 0;

    if (feh_is_image(file, 0)) {
        *im = imlib_load_image_with_error_return(file->filename, &err);
    } else {
        feh_err = LOAD_ERROR_MAGICBYTES;
        err = IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT;
    }

    if (tmpname) {
        *im = imlib_load_image_with_error_return(tmpname, &err);
        if (!err && im) {
            real_filename = file->filename;
            file->filename = tmpname;

            /*
             * feh does not associate a non-native image with its temporary
             * filename and may delete the temporary file right after loading.
             * To ensure that it is still aware of image size, dimensions, etc.,
             * file_info is preloaded here. To avoid a memory leak when loading
             * a non-native file multiple times in a slideshow, the file_info
             * struct is freed first. If file->info is not set,
             * feh_file_info_free is a no-op.
             */
            feh_file_info_free(file->info);
            feh_file_info_load(file, *im);

            file->filename = real_filename;
#ifdef HAVE_LIBEXIF
            /*
             * if we're called from within feh_reload_image, file->ed is already
             * populated.
             */
            if (file->ed) {
                exif_data_unref(file->ed);
            }
            file->ed = exif_data_new_from_file(tmpname);
#endif
        }
        if ((image_source != SRC_HTTP)) unlink(tmpname);

    } else if (im) {
#ifdef HAVE_LIBEXIF
        /*
         * if we're called from within feh_reload_image, file->ed is already
         * populated.
         */
        if (file->ed) {
            exif_data_unref(file->ed);
        }
        file->ed = exif_data_new_from_file(file->filename);
#endif
    }

    if ((err) || (!im)) {
        feh_print_load_error(file->filename, NULL, err, feh_err);
        D(("Load *failed*\n"));
        return (0);
    }

    /*
     * By default, Imlib2 unconditionally loads a cached file without checking
     * if it was modified on disk. However, feh (or rather its users) should
     * expect image changes to appear at the next reload. So we tell Imlib2 to
     * always check the file modification time and only use a cached image if
     * the mtime was not changed. The performance penalty is usually negligible.
     */
    imlib_context_set_image(*im);
    imlib_image_set_changes_on_disk();

#ifdef HAVE_LIBEXIF
    int orientation = 0;
    if (file->ed) {
        ExifByteOrder byteOrder = exif_data_get_byte_order(file->ed);
        ExifEntry *exifEntry =
            exif_data_get_entry(file->ed, EXIF_TAG_ORIENTATION);
        if (exifEntry && opt.auto_rotate) {
            orientation = exif_get_short(exifEntry->data, byteOrder);
        }
    }

    if (orientation == 2)
        gib_imlib_image_flip_horizontal(*im);
    else if (orientation == 3)
        gib_imlib_image_orientate(*im, 2);
    else if (orientation == 4)
        gib_imlib_image_flip_vertical(*im);
    else if (orientation == 5) {
        gib_imlib_image_orientate(*im, 3);
        gib_imlib_image_flip_vertical(*im);
    } else if (orientation == 6)
        gib_imlib_image_orientate(*im, 1);
    else if (orientation == 7) {
        gib_imlib_image_orientate(*im, 3);
        gib_imlib_image_flip_horizontal(*im);
    } else if (orientation == 8)
        gib_imlib_image_orientate(*im, 3);
#endif

    D(("Loaded ok\n"));
    return (1);
}

void im_weprintf(winwidget w, char *fmt, ...) {
    va_list args;
    char *errstr = emalloc(1024);

    fflush(stdout);

    va_start(args, fmt);
    vsnprintf(errstr, 1024, fmt, args);
    va_end(args);

    if (w) w->errstr = errstr;

    fputs(errstr, stderr);
    if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
        fprintf(stderr, " %s", strerror(errno));
    fputs("\n", stderr);
    if (!w) free(errstr);
}

unsigned char reset_output = 0;
