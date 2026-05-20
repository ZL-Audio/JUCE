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

#ifdef JUCE_GUI_BASICS_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "juce_gui_basics.h"

#include "detail/juce_LookAndFeelHelpers.h"

#include "lookandfeel/juce_LookAndFeel.cpp"
#include "lookandfeel/juce_LookAndFeel_V1.cpp"
#include "lookandfeel/juce_LookAndFeel_V2.cpp"
#include "lookandfeel/juce_LookAndFeel_V3.cpp"
#include "lookandfeel/juce_LookAndFeel_V4.cpp"

#include "positioning/juce_MarkerList.cpp"
#include "positioning/juce_RelativeCoordinate.cpp"
#include "positioning/juce_RelativeCoordinatePositioner.cpp"
#include "positioning/juce_RelativeParallelogram.cpp"
#include "positioning/juce_RelativePoint.cpp"
#include "positioning/juce_RelativePointPath.cpp"
#include "positioning/juce_RelativeRectangle.cpp"

#include "drawables/juce_StrokeOptions.cpp"
#include "drawables/juce_Drawable.cpp"
#include "drawables/juce_DrawableComposite.cpp"
#include "drawables/juce_DrawableImage.cpp"
#include "drawables/juce_DrawablePath.cpp"
#include "drawables/juce_DrawableRectangle.cpp"
#include "drawables/juce_DrawableShape.cpp"
#include "drawables/juce_DrawableText.cpp"

#include <juce_gui_basics/detail/juce_LunaSvgFontReplacement.cpp>

// A project may be linking against lunasvg in which case this may already be defined on the command-line
#ifndef LUNASVG_BUILD
 #define LUNASVG_BUILD
#endif
#ifndef LUNASVG_BUILD_STATIC
 #define LUNASVG_BUILD_STATIC
#endif
#define JUCE_PLUTOVG_BUILD
#define JUCE_PLUTOVG_BUILD_STATIC

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4100 4244 4267 6323)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wfloat-conversion",
                                     "-Wfloat-equal",
                                     "-Wmissing-field-initializers",
                                     "-Wshadow-field-in-constructor",
                                     "-Wshadow-uncaptured-local",
                                     "-Wshorten-64-to-32",
                                     "-Wsign-conversion",
                                     "-Wswitch-enum",
                                     "-Wunused-parameter",
                                     "-Wimplicit-int-float-conversion",
                                     "-Wshadow",
                                     "-Wunused-function")

#include "drawables/lunasvg/include/lunasvg.h"
#include "drawables/lunasvg/source/graphics.h"

#include <juce_gui_basics/drawables/lunasvg/source/graphics.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/lunasvg.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svgelement.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svggeometryelement.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svglayoutstate.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svgpaintelement.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svgparser.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svgproperty.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svgrenderstate.cpp>
#include <juce_gui_basics/drawables/lunasvg/source/svgtextelement.cpp>

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

#undef JUCE_PLUTOVG_BUILD_STATIC
#undef JUCE_PLUTOVG_BUILD
#undef LUNASVG_BUILD_STATIC
#undef LUNASVG_BUILD

#include "drawables/juce_SVGParser.cpp"
