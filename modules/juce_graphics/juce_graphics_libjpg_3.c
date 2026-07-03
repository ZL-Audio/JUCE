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

#if ! defined (JUCE_INCLUDE_JPEGLIB_CODE) || JUCE_INCLUDE_JPEGLIB_CODE

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wsign-conversion",
                                     "-Wimplicit-int-conversion",
                                     "-Wunused-parameter",
                                     "-Wswitch-enum")

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4100 4996 6385 28182)

#include "juce_graphics/image_formats/jpglib/jcdctmgr.c"
#include "juce_graphics/image_formats/jpglib/jddctmgr.c"
#include "juce_graphics/image_formats/jpglib/jidctflt.c"

#undef CONST_BITS
#undef DEQUANTIZE
#include "juce_graphics/image_formats/jpglib/jidctfst.c"

#undef CONST_BITS
#undef DEQUANTIZE
#undef FIX_1_847759065
#undef MULTIPLY
#include "juce_graphics/image_formats/jpglib/jidctint.c"

#undef FIX
#include "juce_graphics/image_formats/jpglib/jdmerge.c"

#undef FIX
#include "juce_graphics/image_formats/jpglib/jccolor.c"

#endif
