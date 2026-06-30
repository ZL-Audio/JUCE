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
        for (auto* moduleName : { "SHCore.dll", "User32.dll" })
        {
            LoadLibraryA (moduleName);
            const auto module = GetModuleHandleA (moduleName);

            if (module == nullptr)
                continue;

            using SetProcessDpiAwarenessContextFunc = BOOL (WINAPI*) (DPI_AWARENESS_CONTEXT);

            const auto setProcessDpiAwarenessContext = (SetProcessDpiAwarenessContextFunc) GetProcAddress (module, "SetProcessDpiAwarenessContext");

            if (setProcessDpiAwarenessContext == nullptr)
                continue;

            if (setProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                return true;

            const auto error = GetLastError();

            if (error == ERROR_ACCESS_DENIED)
                return false;

            break;
        }

        if (SUCCEEDED (SetProcessDpiAwareness (PROCESS_PER_MONITOR_DPI_AWARE)))
            return true;

        if (SUCCEEDED (SetProcessDpiAwareness (PROCESS_SYSTEM_DPI_AWARE)))
            return true;

        return SetProcessDPIAware() != 0;
    });

    return didSetDpiAwareness;
}

} // namespace juce
