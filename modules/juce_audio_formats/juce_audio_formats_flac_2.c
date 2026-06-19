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
 #include "juce_audio_formats/codecs/flac/all.h"
 #include "juce_audio_formats/codecs/flac/libFLAC/bitmath.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/bitreader.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/bitwriter.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/cpu.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/crc.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/fixed.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/float.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/format.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/lpc_flac.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/lpc_intrin_avx2.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/lpc_intrin_fma.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/lpc_intrin_neon.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/lpc_intrin_sse2.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/lpc_intrin_sse41.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/md5.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/memory.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/stream_decoder.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/stream_encoder_framing.c"
 #include "juce_audio_formats/codecs/flac/libFLAC/window_flac.c"
#else
 // To get around 'empty translation unit' errors
 enum { placeholder = 0 };
#endif
