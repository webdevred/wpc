/* filelist.c

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
#define _GNU_SOURCE

#include "feh.h"
#include "filelist.h"
#include "signals.h"


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

gib_list *filelist = NULL;
gib_list *original_file_items = NULL; /* original file items from argv */
int filelist_len = 0;
gib_list *current_file = NULL;

static gib_list *rm_filelist = NULL;

feh_file *feh_file_new(char *filename)
{
	feh_file *newfile;
	char *s;

	newfile = (feh_file *) emalloc(sizeof(feh_file));
	newfile->caption = NULL;
	newfile->filename = estrdup(filename);
	s = strrchr(filename, '/');
	if (s)
		newfile->name = estrdup(s + 1);
	else
		newfile->name = estrdup(filename);
	newfile->size = -1;
	newfile->mtime = 0;
	newfile->info = NULL;
#ifdef HAVE_LIBEXIF
	newfile->ed = NULL;
#endif
	return(newfile);
}

feh_file_info *feh_file_info_new(void)
{
	feh_file_info *info;


	info = (feh_file_info *) emalloc(sizeof(feh_file_info));

	info->width = 0;
	info->height = 0;
	info->pixels = 0;
	info->has_alpha = 0;
	info->format = NULL;
	info->extension = NULL;

	return(info);
}

void feh_file_info_free(feh_file_info * info)
{
	if (!info)
		return;
	if (info->format)
		free(info->format);
	if (info->extension)
		free(info->extension);
	free(info);
	return;
}

static void feh_print_stat_error(char *path)
{
	switch (errno) {
	case ENOENT:
	case ENOTDIR:
		weprintf("%s does not exist - skipping", path);
		break;
	case ELOOP:
		weprintf("%s - too many levels of symbolic links - skipping", path);
		break;
	case EACCES:
		weprintf("you don't have permission to open %s - skipping", path);
		break;
	case EOVERFLOW:
		weprintf("Cannot open %s - EOVERFLOW.\n"
			"Recompile with stat64=1 to fix this", path);
		break;
	default:
		weprintf("couldn't open %s", path);
		break;
	}
}



int feh_file_stat(feh_file * file)
{
	struct stat st;

	errno = 0;
	if (stat(file->filename, &st)) {
		feh_print_stat_error(file->filename);
		return(1);
	}

	file->mtime = st.st_mtime;

	file->size = st.st_size;

	return(0);
}

int feh_file_info_load(feh_file * file, Imlib_Image im)
{
	int need_free = 1;
	Imlib_Image im1;

	if (feh_file_stat(file))
		return(1);

	D(("im is %p\n", im));

	if (im)
		need_free = 0;

	if (im)
		im1 = im;
	else if (!feh_load_image(&im1, file) || !im1)
		return(1);

	file->info = feh_file_info_new();

	file->info->width = gib_imlib_image_get_width(im1);
	file->info->height = gib_imlib_image_get_height(im1);

	file->info->has_alpha = gib_imlib_image_has_alpha(im1);

	file->info->pixels = file->info->width * file->info->height;

	file->info->format = estrdup(gib_imlib_image_format(im1));

	if (need_free)
		gib_imlib_free_image_and_decache(im1);
	return(0);
}
