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

/**
    Helper class that owns a Drawable object and exposes it as a DrawableComponent. This class is
    meant to ease the transition from JUCE 8 to JUCE 9, where Drawable no longer inherits from
    Component and DrawableComponent takes no ownership of the Drawable object.

    New projects should avoid using this class if possible.

    The new suggested way to work with Drawable objects is to use them directly and draw them using
    the Drawable::draw() method. This way a heavy dependency on the Component class, and potentially
    the juce_gui_basics module can be avoided.

    @see Drawable

    @tags{GUI}
*/
class JUCE_API  OwningDrawableComponent : private std::unique_ptr<Drawable>,
                                          public DrawableComponent
{
public:
    /** Creates an OwningDrawableComponent that takes ownership of the underlying Drawable object.
    */
    static std::unique_ptr<OwningDrawableComponent> create (std::unique_ptr<Drawable> d)
    {
        if (d == nullptr)
            return {};

        return rawToUniquePtr (new OwningDrawableComponent (std::move (d)));
    }

    /** Creates an OwningDrawableComponent that copies a Drawable and takes ownership of the copied
        object.
    */
    static std::unique_ptr<OwningDrawableComponent> createFromCopy (const Drawable* const d)
    {
        if (d == nullptr)
            return {};

        auto copy = d->createCopy();

        if (copy == nullptr)
            return {};

        return rawToUniquePtr (new OwningDrawableComponent (std::move (copy)));
    }

    /** Attempts to parse an SVG (Scalable Vector Graphics) document from a file.
    */
    static std::unique_ptr<OwningDrawableComponent> createFromSVGFile (const File& svgFile)
    {
        return create (Drawable::createFromSVGFile (svgFile));
    }

    /** Attempts to parse an SVG (Scalable Vector Graphics) document from a string.
    */
    static std::unique_ptr<OwningDrawableComponent> createFromSVGString (const String& svgString)
    {
        return create (Drawable::createFromSVGString (svgString));
    }

private:
    OwningDrawableComponent (std::unique_ptr<Drawable> drawableIn)
        : unique_ptr (std::move (drawableIn)),
          DrawableComponent (*get())
    {
    }
};

} // namespace juce
