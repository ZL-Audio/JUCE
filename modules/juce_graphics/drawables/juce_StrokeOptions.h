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
/** The set of SVG stroke attributes. */
class JUCE_API  StrokeOptions
{
public:
    /** Returns a copy of these attributes with a new colour. */
    [[nodiscard]] StrokeOptions withColour (Colour x) const
    {
        return withMember (*this, &StrokeOptions::colour, x);
    }

    /** Returns a copy of these attributes with a new dash array. */
    [[nodiscard]] StrokeOptions withDashArray (Span<const float> x) const
    {
        std::vector<float> arr (x.begin(), x.end());
        return withMember (*this, &StrokeOptions::dashArray, std::move (arr));
    }

    /** Returns a copy of these attributes with a new dash offset. */
    [[nodiscard]] StrokeOptions withDashOffset (float x) const
    {
        return withMember (*this, &StrokeOptions::dashOffset, x);
    }

    /** Returns a copy of these attributes with a new line cap style. */
    [[nodiscard]] StrokeOptions withLineCap (PathStrokeType::EndCapStyle x) const
    {
        return withMember (*this, &StrokeOptions::lineCap, x);
    }

    /** Returns a copy of these attributes with a new line join style. */
    [[nodiscard]] StrokeOptions withLineJoin (PathStrokeType::JointStyle x) const
    {
        return withMember (*this, &StrokeOptions::lineJoin, x);
    }

    /** Returns a copy of these attributes with a new miter limit. */
    [[nodiscard]] StrokeOptions withMiterLimit (float x) const
    {
        return withMember (*this, &StrokeOptions::miterLimit, x);
    }

    /** Returns a copy of these attributes with a new stroke width. */
    [[nodiscard]] StrokeOptions withWidth (float x) const
    {
        return withMember (*this, &StrokeOptions::width, x);
    }

    /** @see withColour() */
    const auto& getColour() const          { return colour; }

    /** @see withDashArray() */
    Span<const float> getDashArray() const { return dashArray; }

    /** @see withDashOffset() */
    const auto& getDashOffset() const      { return dashOffset; }

    /** @see withLineCap() */
    const auto& getLineCap() const         { return lineCap; }

    /** @see withLineJoin() */
    const auto& getLineJoin() const        { return lineJoin; }

    /** @see withMiterLimit() */
    const auto& getMiterLimit() const      { return miterLimit; }

    /** @see withWidth() */
    const auto& getWidth() const           { return width; }

    /** Creates a PathStrokeType object based on the current attributes. */
    PathStrokeType createStrokeType() const
    {
        PathStrokeType type { width, lineJoin, lineCap };
        type.setMiterLimit (miterLimit);
        return type;
    }

    /** Equality operator. */
    bool operator== (const StrokeOptions& other) const noexcept;

    /** Inequality operator. */
    bool operator!= (const StrokeOptions& other) const noexcept   { return ! operator== (other); }

private:
    Colour colour = Colours::black;  // A combination of the 'stroke' and 'opacity' attributes.
    std::vector<float> dashArray;
    float dashOffset{};
    PathStrokeType::EndCapStyle lineCap = PathStrokeType::EndCapStyle::butt;
    PathStrokeType::JointStyle lineJoin = PathStrokeType::JointStyle::mitered;
    float miterLimit = 4.0f;
    float width = 1.0f;
};

} // namespace juce
