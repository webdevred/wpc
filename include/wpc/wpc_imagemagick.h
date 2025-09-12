#pragma once

#include "wpc/macros.h"

BEGIN_IGNORE_WARNINGS
#ifdef WPC_IMAGEMAGICK_7
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif
END_IGNORE_WARNINGS

__attribute__((used)) static void _wpc_magick_include_marker(void) {
    (void)MagickWandGenesis;
}
