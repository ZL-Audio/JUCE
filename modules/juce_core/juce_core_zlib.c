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

#if ! defined (JUCE_INCLUDE_ZLIB_CODE) || JUCE_INCLUDE_ZLIB_CODE

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wcast-align",
                                     "-Wshorten-64-to-32",
                                     "-Wsign-conversion",
                                     "-Wredundant-decls",
                                     "-Wshadow",
                                     "-Wcomma")

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (6385 6326)

#include "juce_core/zip/zlib/adler32.c"
#include "juce_core/zip/zlib/compress.c"
#include "juce_core/zip/zlib/crc32.c"
#include "juce_core/zip/zlib/deflate.c"
#include "juce_core/zip/zlib/inffast.c"
#undef GZIP
#include "juce_core/zip/zlib/inflate.c"
#include "juce_core/zip/zlib/inftrees.c"
#include "juce_core/zip/zlib/trees.c"
#include "juce_core/zip/zlib/zutil.c"

#endif
