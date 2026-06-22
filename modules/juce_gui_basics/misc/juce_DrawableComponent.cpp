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

DrawableComponent::DrawableComponent (Drawable& drawableIn)
    : drawable (drawableIn)
{
    drawable.addListener (this);
    resetComponentBoundsToDrawable();

    setInterceptsMouseClicks (false, false);
    setPaintingIsUnclipped (true);
    setAccessible (false);
}

DrawableComponent::~DrawableComponent()
{
    drawable.removeListener (this);
}

void DrawableComponent::setTransformToFit (const Rectangle<float>& areaInParent, RectanglePlacement placement)
{
    if (! areaInParent.isEmpty())
        setTransform (placement.getTransformToFit (drawable.getDrawableBounds(), areaInParent));
}

Path DrawableComponent::getOutlineAsPath() const
{
    auto p = drawable.getOutlineAsPath();
    p.applyTransform (getTransform());
    return p;
}

void DrawableComponent::paint (Graphics& g)
{
    g.setOrigin (originRelativeToComponent);
    drawable.draw (g, 1.0f);
}

bool DrawableComponent::hitTest (int x, int y)
{
    if (Component::hitTest (x, y))
        return true;

    bool allowsClicksOnThisComponent, allowsClicksOnChildComponents;
    getInterceptsMouseClicks (allowsClicksOnThisComponent, allowsClicksOnChildComponents);

    if (! allowsClicksOnChildComponents)
        return false;

    const Point<int> p { x - originRelativeToComponent.x, y - originRelativeToComponent.y };
    return drawable.hitTest (p.toFloat());
}

class DrawableTextAccessibilityHandler final : public AccessibilityHandler
{
public:
    DrawableTextAccessibilityHandler (Component& c, const String& textToWrap)
        : AccessibilityHandler (c, AccessibilityRole::staticText),
          text (textToWrap)
    {
    }

    String getTitle() const override
    {
        return text;
    }

private:
    String text;
};

std::unique_ptr<AccessibilityHandler> DrawableComponent::createAccessibilityHandler()
{
    if (drawable.isImage())
        return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image);

    if (auto t = drawable.getContainedText(); t.has_value())
        return std::make_unique<DrawableTextAccessibilityHandler> (*this, *t);

    return Component::createAccessibilityHandler();
}

void DrawableComponent::resetComponentBoundsToDrawable()
{
    const auto drawableBounds = drawable.getDrawableBounds().toNearestInt();
    originRelativeToComponent = -drawableBounds.getPosition();
    setBounds (drawableBounds);
}

void DrawableComponent::drawableBoundsChanged (Drawable*)
{
    resetComponentBoundsToDrawable();
}

} // namespace juce
