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

namespace juce::detail
{

class OwningDrawableComponent : private std::unique_ptr<Drawable>,
                                public DrawableComponent
{
public:
    static std::unique_ptr<OwningDrawableComponent> create (std::unique_ptr<Drawable> d)
    {
        if (d == nullptr)
            return {};

        return rawToUniquePtr (new OwningDrawableComponent (std::move (d)));
    }

    static std::unique_ptr<OwningDrawableComponent> createFromCopy (const Drawable* const d)
    {
        if (d == nullptr)
            return {};

        auto copy = d->createCopy();

        if (copy == nullptr)
            return {};

        return rawToUniquePtr (new OwningDrawableComponent (std::move (copy)));
    }

private:
    OwningDrawableComponent (std::unique_ptr<Drawable> drawableIn)
        : unique_ptr (std::move (drawableIn)),
          DrawableComponent (*get())
    {
    }
};

} // namespace juce::detail
