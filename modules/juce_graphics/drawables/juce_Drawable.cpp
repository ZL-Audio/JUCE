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

Drawable::Drawable (const Drawable& other)
    : transform (other.transform),
      clipPath (other.clipPath != nullptr ? other.clipPath->createCopy() : nullptr),
      name (other.name),
      visible (other.visible)
{
}

void Drawable::applyDrawableClipPath (Graphics& g) const
{
    if (auto* drawable = clipPath.get())
    {
        auto path = drawable->getOutlineAsPath();

        if (! path.isEmpty())
            g.getInternalContext().clipToPath (path, {});
    }
}

//==============================================================================
void Drawable::draw (Graphics& g, float opacity, const AffineTransform& drawTransform) const
{
    Graphics::ScopedSaveState ss (g);

    g.addTransform (getTransform().followedBy (drawTransform));

    applyDrawableClipPath (g);

    if (! g.isClipEmpty())
    {
        if (opacity < 1.0f)
        {
            g.beginTransparencyLayer (opacity);
            paint (g);
            g.endTransparencyLayer();
        }
        else
        {
            paint (g);
        }
    }
}

void Drawable::drawAt (Graphics& g, float x, float y, float opacity) const
{
    draw (g, opacity, AffineTransform::translation (x, y));
}

void Drawable::drawWithin (Graphics& g, Rectangle<float> destArea,
                           RectanglePlacement placement, float opacity) const
{
    draw (g, opacity, placement.getTransformToFit (getDrawableBounds(), destArea));
}

//==============================================================================
void Drawable::setClipPath (std::unique_ptr<Drawable> clipPathIn)
{
    clipPath = std::move (clipPathIn);
}

//==============================================================================
bool Drawable::replaceColour (Colour, Colour)
{
    return false;
}

void Drawable::setDrawableTransform (const AffineTransform& newTransform)
{
    transform = newTransform;
}

AffineTransform Drawable::getDrawableTransform() const
{
    return transform;
}

AffineTransform Drawable::getTransform() const
{
    return transform;
}

//==============================================================================
std::unique_ptr<Drawable> Drawable::createFromImageData (const void* data, const size_t numBytes)
{
    auto image = ImageFileFormat::loadFrom (data, numBytes);

    if (image.isValid())
        return std::make_unique<DrawableImage> (image);

    if (const auto s = String::createStringFromData (data, (int) numBytes); s.contains ("<svg"))
        return Drawable::createFromSVGString (s);

    return {};
}

std::unique_ptr<Drawable> Drawable::createFromImageDataStream (InputStream& dataSource)
{
    MemoryOutputStream mo;
    mo << dataSource;

    return createFromImageData (mo.getData(), mo.getDataSize());
}

std::unique_ptr<Drawable> Drawable::createFromImageFile (const File& file)
{
    FileInputStream fin (file);

    if (fin.openedOk())
        return createFromImageDataStream (fin);

    return {};
}

} // namespace juce
