/*
  ==============================================================================

   This file is part of the JUCE 9 preview.
   Copyright (c) Raw Material Software Limited

   You may use this code under the terms of the AGPLv3
   (see www.gnu.org/licenses).

   For the JUCE 9 preview this file cannot be licensed commercially.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <juce_core/system/juce_CompilerWarnings.h>

#if JUCE_INCLUDE_PNGLIB_CODE || ! defined (JUCE_INCLUDE_PNGLIB_CODE)

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wsign-conversion",
                                     "-Wfloat-equal",
                                     "-Wshadow",
                                     "-Wshorten-64-to-32",
                                     "-Wdeprecated-declarations",
                                     "-Wredundant-decls")

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4146 4996)

#include "juce_graphics/image_formats/pnglib/png.c"
#include "juce_graphics/image_formats/pnglib/pngerror.c"
#include "juce_graphics/image_formats/pnglib/pngget.c"
#include "juce_graphics/image_formats/pnglib/pngmem.c"
#include "juce_graphics/image_formats/pnglib/pngread.c"
#include "juce_graphics/image_formats/pnglib/pngpread.c"
#include "juce_graphics/image_formats/pnglib/pngrio.c"
#include "juce_graphics/image_formats/pnglib/pngrtran.c"

#define png_pass_start png_pass_start_2
#define png_pass_inc png_pass_inc_2
#define png_pass_ystart png_pass_ystart_2
#define png_pass_yinc png_pass_yinc_2

#include "juce_graphics/image_formats/pnglib/pngrutil.c"

#undef png_pass_start
#undef png_pass_inc
#undef png_pass_ystart
#undef png_pass_yinc

#include "juce_graphics/image_formats/pnglib/pngset.c"
#include "juce_graphics/image_formats/pnglib/pngtrans.c"
#include "juce_graphics/image_formats/pnglib/pngwio.c"
#include "juce_graphics/image_formats/pnglib/pngwrite.c"
#include "juce_graphics/image_formats/pnglib/pngwtran.c"

#define png_pass_start png_pass_start_3
#define png_pass_inc png_pass_inc_3
#define png_pass_ystart png_pass_ystart_3
#define png_pass_yinc png_pass_yinc_3

#include "juce_graphics/image_formats/pnglib/pngwutil.c"

#undef png_pass_start
#undef png_pass_inc
#undef png_pass_ystart
#undef png_pass_yinc

#include "juce_graphics/image_formats/pnglib/arm/arm_init.c"
#include "juce_graphics/image_formats/pnglib/arm/filter_neon_intrinsics.c"
#include "juce_graphics/image_formats/pnglib/arm/palette_neon_intrinsics.c"

#endif
