#pragma once

#ifdef WPC_IMAGEMAGICK_7
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif

__attribute__((used)) static void _wpc_magick_include_marker(void) {
    (void)MagickWandGenesis;
}
