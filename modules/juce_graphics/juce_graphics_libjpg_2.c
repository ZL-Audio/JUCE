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
                                     "-Wswitch-enum",
                                     "-Wdeprecated-declarations",
                                     "-Wcomma")

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4100 4996 6385 6386 6387)

#include "juce_graphics/image_formats/jpglib/jdapimin.c"
#include "juce_graphics/image_formats/jpglib/jdapistd.c"
#include "juce_graphics/image_formats/jpglib/jdarith.c"
#include "juce_graphics/image_formats/jpglib/jdcoefct.c"
#include "juce_graphics/image_formats/jpglib/jdcolor.c"
#include "juce_graphics/image_formats/jpglib/jdmainct.c"
#include "juce_graphics/image_formats/jpglib/jdmarker.c"
#include "juce_graphics/image_formats/jpglib/jdmaster.c"
#include "juce_graphics/image_formats/jpglib/jdpostct.c"
#include "juce_graphics/image_formats/jpglib/jquant1.c"
#include "juce_graphics/image_formats/jpglib/jdsample.c"

#endif
