/*
    Experimental embedded-editor keyboard routing for JUCE commit
    2841786b67f26a956e6c334f9274c0f4a9024d9d.

    Included inside namespace juce, after LinuxComponentPeer is defined.
    Requires the JUCE/X11 types already used by juce_XWindowSystem_linux.cpp.
    This is a downstream policy for mouse-first plug-ins, not a universal
    implementation of host keyboard routing or the full XEmbed protocol.
*/

#ifndef JUCE_LINUX_EMBEDDED_KEY_FORWARDING
 #define JUCE_LINUX_EMBEDDED_KEY_FORWARDING 1
#endif

namespace EmbeddedKeyboardRouting
{
    enum class Destination { unset, host, local };

    struct HeldKey
    {
        Destination destination = Destination::unset;
        WeakReference<Component> owner;
        ::Window hostWindow = None;
    };

    using HeldKeys = std::array<HeldKey, 256>;

    static auto& getHeldKeys()
    {
        static std::unordered_map<LinuxComponentPeer*, HeldKeys> state;
        return state;
    }

    static void forgetPeer (LinuxComponentPeer* peer)
    {
        getHeldKeys().erase (peer);
    }

    static bool isRealFocusChange (const XFocusChangeEvent& event)
    {
        // Do not discard NotifyInferior, or genuine changes made while a
        // keyboard grab exists. Grab activation/deactivation is different.
        return (event.mode == NotifyNormal || event.mode == NotifyWhileGrabbed)
            && event.detail != NotifyPointer
            && event.detail != NotifyPointerRoot;
    }

    static Component* getKeyboardOwner (LinuxComponentPeer& peer)
    {
        auto* focused = Component::getCurrentlyFocusedComponent();

        if (focused == nullptr || focused->getPeer() != &peer || ! focused->isShowing())
            return nullptr;

        // An active TextInputTarget owns the entire key, even if it rejects
        // a particular character. Rejected numerical input must not start
        // playback or trigger other host shortcuts.
        if (peer.findCurrentTextInputTarget() != nullptr)
            return focused;

        // Optional opt-in for a focused non-text component (e.g. a custom
        // keyboard-operated control). Ordinary ComboBoxes are not opted in.
        if ((bool) focused->getProperties()["juceLinuxOwnsKeyboard"])
            return focused;

        return nullptr;
    }

    static bool forwardToHost (LinuxComponentPeer& peer,
                               const XKeyEvent& original,
                               ::Window destination)
    {
        // Synthetic events may already have been delegated by the host.
        // Do not bounce them back: that can create host<->plug-in loops.
        // A host that delegates all keys must itself give host shortcuts
        // first refusal. send_event does NOT prove that it did so.
        if (original.send_event
            || destination == None
            || destination == original.root
            || destination == peer.getWindowHandle()
            || destination != peer.getParentWindow())
        {
            return false;
        }

        auto* display = XWindowSystem::getInstance()->getDisplay();
        if (display == nullptr)
            return false;

        const XWindowSystemUtilities::ScopedXLock lock;
        auto* symbols = X11Symbols::getInstance();

        XEvent outgoing {};
        outgoing.xkey = original;
        outgoing.xkey.display = display;
        outgoing.xkey.window = destination;
        outgoing.xkey.subwindow = None;

        ::Window child = None;
        if (! symbols->xTranslateCoordinates (display,
                                              original.window,
                                              destination,
                                              original.x,
                                              original.y,
                                              &outgoing.xkey.x,
                                              &outgoing.xkey.y,
                                              &child))
        {
            return false;
        }

        // Deliver to the X client that created the embedding window.
        // Never propagate to an arbitrary ancestor (in particular the root).
        const auto status = symbols->xSendEvent (display, destination, False,
                                                 NoEventMask, &outgoing);
        symbols->xFlush (display);
        // This reports request submission, NOT whether the DAW handled it.
        return status != 0;
    }

    static bool dispatchLocally (LinuxComponentPeer* peer,
                                 const XKeyEvent& event,
                                 bool releaseIsAutoRepeat = false)
    {
        if (! JUCE_LINUX_EMBEDDED_KEY_FORWARDING || ! peer->shouldDeferFocusToEmbedder())
            return true;

        if (event.keycode >= 256)
            return false;

        auto& held = getHeldKeys()[peer][event.keycode];
        if (held.destination == Destination::unset)
        {
            auto* owner = getKeyboardOwner (*peer);
            held.destination = owner != nullptr ? Destination::local
                                                 : Destination::host;
            held.owner = owner;
            held.hostWindow = peer->getParentWindow();
        }

        // Snapshot before clearing: callbacks triggered by a local key may
        // close the field or delete this peer. No state reference escapes.
        const auto destination = held.destination;
        const auto owner = held.owner;
        const auto host = held.hostWindow;

        if (event.type == KeyRelease && ! releaseIsAutoRepeat)
            held = HeldKey {};

        if (destination == Destination::host)
        {
            forwardToHost (*peer, event, host);
            return false;
        }

        // If Return closed the editor on key-down, its key-up is swallowed,
        // not reassigned to the host or a newly focused control. The same
        // rule holds for auto-repeat while the original owner disappears.
        auto* target = owner.get();
        return target != nullptr
            && target == Component::getCurrentlyFocusedComponent()
            && target->getPeer() == peer
            && target->isShowing();
    }
}
