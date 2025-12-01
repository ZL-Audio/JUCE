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

/** @cond */
/** @internal
    Helper methods for sorting FontFeatureSettings and FontVariableSettings by tag.
*/
struct FontComparators
{
    FontComparators() = delete;

    struct FeatureSettingComparator
    {
        constexpr bool operator() (FontFeatureSetting a, FontFeatureSetting b)   const { return a.tag < b.tag; }
        constexpr bool operator() (FontFeatureTag a, FontFeatureSetting b)       const { return a < b.tag; }
        constexpr bool operator() (FontFeatureSetting a, FontFeatureTag b)       const { return a.tag < b; }
    };

    struct VariableSettingComparator
    {
        constexpr bool operator() (FontVariableSetting a, FontVariableSetting b) const { return a.tag < b.tag; }
        constexpr bool operator() (FontFeatureTag a, FontVariableSetting b)      const { return a < b.tag; }
        constexpr bool operator() (FontVariableSetting a, FontFeatureTag b)      const { return a.tag < b; }
    };
};
/** @endcond */

} // namespace juce
