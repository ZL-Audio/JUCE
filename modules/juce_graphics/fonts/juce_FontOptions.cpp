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

struct FontValues
{
    static float limitFontHeight (const float height) noexcept
    {
        return jlimit (0.1f, 10000.0f, height);
    }

    inline static constexpr float defaultFontHeight = 14.0f;
    static float minimumHorizontalScale;

    FontValues() = delete;
};

float FontValues::minimumHorizontalScale = 0.7f;

FontOptions::FontOptions()
    : FontOptions (FontValues::defaultFontHeight, Font::plain)
{
}

FontOptions::FontOptions (float fontHeight)
    : FontOptions (fontHeight, Font::plain)
{
}

FontOptions::FontOptions (float fontHeight, int styleFlags)
    : FontOptions ({}, fontHeight, styleFlags)
{
}

FontOptions::FontOptions (const String& typefaceName, float fontHeight, int styleFlags)
    : FontOptions (typefaceName, FontStyleHelpers::getStyleName (styleFlags), fontHeight)
{
    underlined = (styleFlags & Font::FontStyleFlags::underlined) != 0;
}

FontOptions::FontOptions (const String& typefaceName, const String& typefaceStyle, float fontHeight)
    : name (typefaceName),
      style (typefaceStyle),
      height (FontValues::limitFontHeight (fontHeight))
{
}

FontOptions::FontOptions (const Typeface::Ptr& ptr)
    : name (ptr->getName()),
      style (ptr->getStyle()),
      typeface (ptr),
      height (FontValues::defaultFontHeight)
{
}

FontOptions FontOptions::withStyleFlags (int x) const
{
    if (typeface == nullptr)
    {
        auto copy = withMember (*this, &FontOptions::style, FontStyleHelpers::getStyleName (x));
        copy.underlined = (x & Font::FontStyleFlags::underlined) != 0;
        return copy;
    }

    // This field will be ignored if the typeface pointer is non-null.
    // If you want to set a custom style, first set the typeface pointer to null.
    jassertfalse;
    return *this;
}

auto FontOptions::tie() const
{
    return std::tuple (name,
                       style,
                       typeface.get(),
                       fallbacks,
                       features,
                       variables,
                       metricsKind,
                       ascentOverride,
                       descentOverride,
                       height,
                       pointHeight,
                       tracking,
                       horizontalScale,
                       fallbackEnabled,
                       underlined);
}

FontOptions FontOptions::withFeatureSetting (FontFeatureSetting newSetting) const
{
    auto copy = *this;

    OrderedContainerHelpers::insertOrAssign (copy.features,
                                             newSetting,
                                             FontComparators::FeatureSettingComparator{});

    return copy;
}

FontOptions FontOptions::withFeatureRemoved (FontFeatureTag featureTag) const
{
    auto copy = *this;

    OrderedContainerHelpers::remove (copy.features,
                                     featureTag,
                                     FontComparators::FeatureSettingComparator{});

    return copy;
}

FontOptions FontOptions::withVariableSetting (FontVariableSetting variableSetting) const
{
    jassert (std::isfinite (variableSetting.value));

    if (typeface == nullptr && style.isEmpty())
    {
        auto copy = *this;
        OrderedContainerHelpers::insertOrAssign (copy.variables,
                                                 variableSetting,
                                                 FontComparators::VariableSettingComparator{});
        return copy;
    }

    // Use Typeface::cloneWithVariableSettings to configure an existing typeface.
    jassertfalse;
    return *this;
}

FontOptions FontOptions::withVariableSettings (Span<const FontVariableSetting> variableSettings) const
{
    if (variableSettings.empty() || (typeface == nullptr && style.isEmpty()))
    {
        auto copy = *this;
        copy.variables = { variableSettings.begin(), variableSettings.end() };
        std::sort (copy.variables.begin(),
                   copy.variables.end(),
                   FontComparators::VariableSettingComparator{});

        return copy;
    }

    // Use Typeface::cloneWithVariableSettings to configure an existing typeface.
    jassertfalse;
    return *this;
}

FontOptions FontOptions::withVariableRemoved (FontFeatureTag featureTag) const
{
    if (typeface == nullptr && style.isEmpty())
    {
        auto copy = *this;
        OrderedContainerHelpers::remove (copy.variables,
                                         featureTag,
                                         FontComparators::VariableSettingComparator{});
        return copy;
    }

    // Use Typeface::cloneWithVariableSettings to configure an existing typeface.
    jassertfalse;
    return *this;
}

bool FontOptions::operator== (const FontOptions& other) const { return tie() == other.tie(); }
bool FontOptions::operator!= (const FontOptions& other) const { return tie() != other.tie(); }
bool FontOptions::operator<  (const FontOptions& other) const { return tie() <  other.tie(); }
bool FontOptions::operator<= (const FontOptions& other) const { return tie() <= other.tie(); }
bool FontOptions::operator>  (const FontOptions& other) const { return tie() >  other.tie(); }
bool FontOptions::operator>= (const FontOptions& other) const { return tie() >= other.tie(); }

#if JUCE_UNIT_TESTS

class FontFeatureContainerTests : public UnitTest
{
public:
    FontFeatureContainerTests() : UnitTest ("FontFeatureContainerTests", UnitTestCategories::fonts)
    {
    }

    void runTest() override
    {
        beginTest ("Features can be enabled");
        {
            const auto options = FontOptions{}.withFeatureEnabled ("clig");

            expectEquals ((int) options.getFeatureSettings().size(), 1);
            expect (compareFeatureLists (options.getFeatureSettings(),
            {
                FontFeatureSetting ("clig", FontFeatureSetting::featureEnabled)
            }));
        }

        beginTest ("Features can be disabled");
        {
            const auto options = FontOptions{}.withFeatureDisabled ("clig");

            expectEquals ((int) options.getFeatureSettings().size(), 1);
            expect (compareFeatureLists (options.getFeatureSettings(),
            {
                FontFeatureSetting ("clig", FontFeatureSetting::featureDisabled)
            }));
        }

        beginTest ("Features can be removed");
        {
            const auto options = FontOptions{}.withFeatureEnabled ("clig")
                                              .withFeatureRemoved ("clig");

            expectEquals ((int) options.getFeatureSettings().size(), 0);
            expect (compareFeatureLists (options.getFeatureSettings(), {}));
        }

        beginTest ("Duplicate features are not allowed");
        {
            const auto options = FontOptions{}.withFeatureEnabled ("clig")
                                              .withFeatureEnabled ("clig");

            expectEquals ((int) options.getFeatureSettings().size(), 1);
        }

        beginTest ("Features are always sorted by tag");
        {
            const auto options = FontOptions{}.withFeatureEnabled ("clig")
                                              .withFeatureDisabled ("blig")
                                              .withFeatureEnabled ("alig");

            expectEquals ((int) options.getFeatureSettings().size(), 3);
            expect (compareFeatureLists (options.getFeatureSettings(),
            {
                FontFeatureSetting ("alig", FontFeatureSetting::featureEnabled),
                FontFeatureSetting ("blig", FontFeatureSetting::featureDisabled),
                FontFeatureSetting ("clig", FontFeatureSetting::featureEnabled)
            }));
        }
    }

private:
    static bool compareFeatureLists (Span<const FontFeatureSetting> input,
                                     std::initializer_list<const FontFeatureSetting> expected)
    {
        return std::equal (input.begin(), input.end(), expected.begin(), expected.end());
    }
};

static FontFeatureContainerTests fontFeatureContainerTests;

#endif


} // namespace juce
