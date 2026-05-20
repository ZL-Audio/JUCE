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
    A drawable object which acts as a container for a set of other Drawables.

    Note that although this is a Component, it takes ownership of its child components
    and will delete them, so that you can use it as a self-contained graphic object.
    The intention is that you should not add your own components to it, only add other
    Drawable objects.

    @see Drawable

    @tags{GUI}
*/
class JUCE_API  DrawableComposite  : public Drawable
{
public:
    //==============================================================================
    /** Creates a composite Drawable. */
    DrawableComposite() = default;

    /** Creates a copy of a DrawableComposite. */
    DrawableComposite (const DrawableComposite&);

    //==============================================================================
    /** Sets the parallelogram that defines the target position of the content rectangle when the drawable is rendered.
        @see setContentArea
    */
    void setBoundingBox (Parallelogram<float> newBoundingBox);

    /** Sets the rectangle that defines the target position of the content rectangle when the drawable is rendered.
        @see setContentArea
    */
    void setBoundingBox (Rectangle<float> newBoundingBox);

    /** Returns the parallelogram that defines the target position of the content rectangle when the drawable is rendered.
        @see setBoundingBox
    */
    Parallelogram<float> getBoundingBox() const noexcept            { return bounds; }

    /** Changes the bounding box transform to match the content area, so that any sub-items will
        be drawn at their untransformed positions.
    */
    void resetBoundingBoxToContentArea();

    /** Returns the main content rectangle.
        @see contentLeftMarkerName, contentRightMarkerName, contentTopMarkerName, contentBottomMarkerName
    */
    Rectangle<float> getContentArea() const noexcept                { return contentArea; }

    /** Changes the main content area.
        @see setBoundingBox, contentLeftMarkerName, contentRightMarkerName, contentTopMarkerName, contentBottomMarkerName
    */
    void setContentArea (Rectangle<float> newArea);

    /** Resets the content area and the bounding transform to fit around the area occupied
        by the child components.
    */
    void resetContentAreaAndBoundingBoxToFitChildren();

    bool replaceColour (Colour originalColour, Colour replacementColour) override;

    bool hitTest (Point<float> p) const override;

    /** Adds a child Drawable to this one. */
    void addChild (std::unique_ptr<Drawable> newChild);

    /** Returns the number of child Drawables that this composite contains.
    */
    int getNumChildren() const
    {
        return static_cast<int> (children.size());
    }

    Drawable& getChild (int index)
    {
        jassert (isPositiveAndBelow (index, children.size()));
        return *children[(size_t) index];
    }

    /** Returns a child Drawable by its index.
    */
    const Drawable& getChild (int index) const
    {
        jassert (isPositiveAndBelow (index, children.size()));
        return *children[(size_t) index];
    }

    void setBlendMode (BlendMode newBlendMode)
    {
        blendMode = newBlendMode;
    }

    void setLuminanceMask (bool isMask)
    {
        luminanceMask = isMask;
    }

    void setOpacity (float newOpacity)
    {
        opacity = newOpacity;
    }

    //==============================================================================
    /** @internal */
    std::unique_ptr<Drawable> createCopy() const override;
    /** @internal */
    Rectangle<float> getDrawableBounds() const override;
    /** @internal */
    Path getOutlineAsPath() const override;

private:
    void paint (Graphics& g) const override;

    bool requiresBlendBuffer() const override;

    //==============================================================================
    std::vector<std::unique_ptr<Drawable>> children;
    Rectangle<float> contentArea { 0.0f, 0.0f, 100.0f, 100.0f };
    Parallelogram<float> bounds { contentArea };
    BlendMode blendMode;
    bool luminanceMask = false;
    float opacity = 1.0f;

    JUCE_LEAK_DETECTOR (DrawableComposite)
};

} // namespace juce
