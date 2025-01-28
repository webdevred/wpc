/* include X11 stuff */
#include <X11/Xlib.h>
/* include Imlib2 stuff */
#include <Imlib2.h>
/* sprintf include */
#include <stdio.h>
#include <stdlib.h>

/* some globals for our window & X display */
Display *disp;
Window   win;
Visual  *vis;
Colormap cm;
int      depth;

/* the program... */
int main(int argc, char **argv)
{
   disp  = XOpenDisplay(NULL);
   vis   = DefaultVisual(disp, DefaultScreen(disp));
   depth = DefaultDepth(disp, DefaultScreen(disp));
   cm    = DefaultColormap(disp, DefaultScreen(disp));
   win = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 
                             0, 0, 640, 480, 0, 0, 0);
   XSelectInput(disp, win, ButtonPressMask | ButtonReleaseMask | 
                PointerMotionMask | ExposureMask);
   XMapWindow(disp, win);
   imlib_set_cache_size(2048 * 1024);
   imlib_set_font_cache_size(512 * 1024);
   imlib_add_path_to_font_path("./ttfonts");
   imlib_set_color_usage(128);
   imlib_context_set_dither(1);
   imlib_context_set_display(disp);
   imlib_context_set_visual(vis);
   imlib_context_set_colormap(cm);
   imlib_context_set_drawable(win);

   Imlib_Image image;
   
   image = imlib_load_image("./bakgrund.jpg");
   imlib_context_set_image(image);

   imlib_context_set_image(image);


   imlib_context_set_blend(0);

   imlib_render_image_on_drawable(0,0);

   imlib_free_image();
   for (;;) {
   }
   return 0;
}
