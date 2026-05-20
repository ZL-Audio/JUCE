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

#define JUCE_PLUTOVG_BUILD
#define JUCE_PLUTOVG_BUILD_STATIC

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4018 4100 4244 4267 4324 4389 4996 6011 6308 6387 6385 6386 6262 28182)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wcast-align",
                                     "-Wfloat-conversion",
                                     "-Wfloat-equal",
                                     "-Wimplicit-int-conversion",
                                     "-Wmissing-prototypes",
                                     "-Wshorten-64-to-32",
                                     "-Wsign-compare",
                                     "-Wsign-conversion",
                                     "-Wswitch-enum",
                                     "-Wunused-function",
                                     "-Wunused-parameter",
                                     "-Wcomma",
                                     "-Wimplicit-int-float-conversion",
                                     "-Wunused-but-set-variable")

#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-blend.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-canvas.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-font.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-ft-math.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-ft-raster.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-ft-stroker.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-matrix.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-paint.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-path.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-rasterize.c>
#include <juce_graphics/drawables/lunasvg/plutovg/source/plutovg-surface.c>

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

#undef JUCE_PLUTOVG_BUILD_STATIC
#undef JUCE_PLUTOVG_BUILD
