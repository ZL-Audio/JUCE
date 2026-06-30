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

bool HiddenMessageWindow::setDPIAwareness()
{
    static const auto didSetDpiAwareness = std::invoke ([]
    {
        constexpr auto shcore = "SHCore.dll";
        LoadLibraryA (shcore);

        const auto shcoreModule = GetModuleHandleA (shcore);

        if (shcoreModule == nullptr)
            return false;

        using SetProcessDpiAwarenessContextFunc = BOOL (WINAPI*) (DPI_AWARENESS_CONTEXT);
        const auto setProcessDpiAwarenessContext = (SetProcessDpiAwarenessContextFunc) GetProcAddress (shcoreModule, "SetProcessDpiAwarenessContext");

        if (setProcessDpiAwarenessContext != nullptr
            && setProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            return true;

        if (SUCCEEDED (SetProcessDpiAwareness (PROCESS_PER_MONITOR_DPI_AWARE)))
            return true;

        if (SUCCEEDED (SetProcessDpiAwareness (PROCESS_SYSTEM_DPI_AWARE)))
            return true;

        return SetProcessDPIAware() != 0;
    });

    return didSetDpiAwareness;
}

} // namespace juce
