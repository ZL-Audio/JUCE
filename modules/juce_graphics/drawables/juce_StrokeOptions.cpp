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

namespace juce
{

bool StrokeOptions::operator== (const StrokeOptions& other) const noexcept
{
    const auto tie = [] (auto& x)
    {
        return std::tie (x.colour,
                         x.dashArray,
                         x.dashOffset,
                         x.lineCap,
                         x.lineJoin,
                         x.miterLimit,
                         x.width);
    };

    return tie (*this) == tie (other);
}

} // namespace juce
