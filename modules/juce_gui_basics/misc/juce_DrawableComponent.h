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

class JUCE_API DrawableComponent : public Component,
                                   private Drawable::Listener
{
public:
    /** Wraps a Drawable object in a Component. */
    explicit DrawableComponent (Drawable& drawableIn);

    /** Destructor. */
    ~DrawableComponent() override;

    /** Sets a transform for this drawable that will position it within the specified
        area of its parent component.
    */
    void setTransformToFit (const Rectangle<float>& areaInParent, RectanglePlacement placement);

    /** Creates a path that describes the outline of this DrawableComponent. */
    Path getOutlineAsPath() const;

    /** Returns a reference to the Drawable object displayed by this Component. */
    Drawable& getDrawable()
    {
        return drawable;
    }

    /** Returns a reference to the Drawable object displayed by this Component. */
    const Drawable& getDrawable() const
    {
        return drawable;
    }

    //==============================================================================
    void paint (Graphics& g) override;

    bool hitTest (int x, int y) override;

    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

    //==============================================================================
    /** @internal

        Intended to replace Drawable::setBoundsToEnclose which is only ever called in conjunction
        with getDrawableBounds().
    */
    void resetComponentBoundsToDrawable();

private:
    void drawableBoundsChanged (Drawable*) override;

    Drawable& drawable;
    Point<int> originRelativeToComponent;
};

} // namespace juce
