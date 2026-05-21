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

#pragma once

#include <juce_audio_plugin_client/detail/juce_IncludeModuleHeaders.h>
#include <juce_audio_plugin_client/detail/juce_CreatePluginFilter.h>

namespace juce::detail
{

struct PluginUtilities
{
    PluginUtilities() = delete;

    struct FlagsAndWindowsMultiTouch
    {
        FlagsAndWindowsMultiTouch (int d, bool w)
            : desktopFlags (d),
              windowsUsesMultiTouch (w)
        {
        }

        int desktopFlags{};
        bool windowsUsesMultiTouch{};
    };

    static FlagsAndWindowsMultiTouch getDesktopFlagsAndWindowsMultiTouchMode (const AudioProcessorEditor& editor)
    {
        auto flags = editor.wantsLayerBackedView()
                   ? 0
                   : ComponentPeer::windowRequiresSynchronousCoreGraphicsRendering;

        return { flags, editor.usesWindowsMultiTouch() };
    }

    static FlagsAndWindowsMultiTouch getDesktopFlagsAndWindowsMultiTouchMode (const AudioProcessorEditor* editor)
    {
        return editor != nullptr ? getDesktopFlagsAndWindowsMultiTouchMode (*editor)
                                 : FlagsAndWindowsMultiTouch { 0, false };
    }

    static void addToDesktop (AudioProcessorEditor& editor, void* parent)
    {
        const auto [flags, usesWindowsMultiTouch] = getDesktopFlagsAndWindowsMultiTouchMode (editor);
        editor.addToDesktop (flags, parent);

        if (auto* peer = editor.getPeer())
            peer->setWindowsCanUseMultiTouch (usesWindowsMultiTouch);
    }

    static const PluginHostType& getHostType()
    {
        static PluginHostType hostType;
        return hostType;
    }
};

} // namespace juce::detail
