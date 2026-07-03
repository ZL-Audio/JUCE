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

#include "juce_audio_formats/codecs/flac/juce_flac_config.h"

#if JUCE_USE_FLAC && (JUCE_INCLUDE_FLAC_CODE || ! defined (JUCE_INCLUDE_FLAC_CODE))

 JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")

 #include "juce_audio_formats/codecs/flac/libFLAC/stream_encoder.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/stream_encoder_intrin_avx2.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/stream_encoder_intrin_sse2.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/stream_encoder_intrin_ssse3.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/fixed_intrin_avx2.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/fixed_intrin_sse2.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/fixed_intrin_sse42.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/fixed_intrin_ssse3.c"

 #ifdef _WIN32
  #include "juce_audio_formats/codecs/flac/win_utf8_io/win_utf8_io.c"
 #endif
#else
 // To get around 'empty translation unit' errors
 enum { placeholder = 0 };
#endif
