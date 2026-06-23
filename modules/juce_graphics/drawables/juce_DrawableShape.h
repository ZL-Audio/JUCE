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

//==============================================================================
/**
    A base class implementing common functionality for Drawable classes which
    consist of some kind of filled and stroked outline.

    @see DrawablePath, DrawableRectangle

    @tags{GUI}
*/
class JUCE_API  DrawableShape   : public Drawable
{
protected:
    //==============================================================================
    DrawableShape() = default;
    DrawableShape (const DrawableShape&) = default;

public:
    //==============================================================================
    /** Sets a fill type for the path.
        This colour is used to fill the path - if you don't want the path to be
        filled (e.g. if you're just drawing an outline), set this to a transparent
        colour.

        @see setPath, setStrokeFill
    */
    void setFill (const FillType& newFill);

    /** Returns the current fill type.
        @see setFill
    */
    const FillType& getFill() const noexcept                { return mainFill; }

    /** Sets the fill type with which the outline will be drawn.
        @see setFill
    */
    void setStrokeFill (const FillType& newStrokeFill);

    /** Returns the current stroke fill.
        @see setStrokeFill
    */
    const FillType& getStrokeFill() const noexcept          { return strokeFill; }

    /** Changes the properties of the outline that will be drawn around the path.
        If the stroke has 0 thickness, no stroke will be drawn.
        @see setStrokeThickness, setStrokeColour
    */
    void setStrokeType (const PathStrokeType& newStrokeType);

    /** Changes the stroke thickness.
        This is a shortcut for calling setStrokeType.
    */
    void setStrokeThickness (float newThickness);

    /** Returns the current outline style. */
    PathStrokeType getStrokeType() const noexcept           { return strokeOptions.createStrokeType(); }

    /** Provides a set of dash lengths to use for stroking the path. */
    void setDashLengths (Span<const float> newDashLengths);

    /** Provides an offset on the rendering of the associated dash array. */
    void setDashOffset (float dashOffset);

    /** Returns the set of dash lengths that the path is using. */
    Span<const float> getDashLengths() const&               { return strokeOptions.getDashArray(); }
    Span<const float> getDashLengths() const&& = delete;

    //==============================================================================
    /** @internal */
    Rectangle<float> getDrawableBoundsUntransformed() const override;
    /** @internal */
    bool hitTest (Point<float>) const override;
    /** @internal */
    bool replaceColour (Colour originalColour, Colour replacementColour) override;
    /** @internal */
    Path getOutlineAsPath() const override;

protected:
    //==============================================================================
    /** Called when the cached path should be updated. */
    void pathChanged();
    /** Called when the cached stroke should be updated. */
    void strokeChanged();
    /** True if there's a stroke with a non-zero thickness and non-transparent colour. */
    bool isStrokeVisible() const noexcept;

    //==============================================================================
    StrokeOptions strokeOptions = StrokeOptions{}.withWidth (0.0f);
    Path path, strokePath;

private:
    void paint (Graphics& g) const override;

    FillType mainFill, strokeFill;
};

} // namespace juce
