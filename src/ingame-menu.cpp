//
// Created by styyx on 04/03/2026.
//

#include "inputmanager.h"
#include "Settings.h"

namespace
{
    // SKSE/CLibUtil macro keycodes used by Stances NG:
    // LB = 274, Gamepad X = 278.
    constexpr std::uint32_t kLeftBumperKey = 274;
    constexpr std::uint32_t kGamepadXKey = 278;
    constexpr std::uint32_t kGamepadXMask = 0x4000;
}

void STNG::InputEventListener::RegisterInput()
{
    if (const auto manager = RE::BSInputDeviceManager::GetSingleton()) {
        // Run before vanilla PlayerControls so we can selectively consume
        // the mapped Ready Weapon event produced by X when LB+X is a stance combo.
        manager->PrependEventSink(GetSingleton());
        SKSE::log::info("[[REGISTERED]] for {}", typeid(RE::InputEvent).name());
    }
}

void STNG::InputEventListener::SetKeys()
{
    using s = Config::Settings;
    const auto i = GetSingleton();

    if (!i->hotkey_neutral.SetPattern(s::neutral_stance_key.GetValue()))
    {
        logs::error("neutral stance key set failed");
    }
    if (!i->hotkey_bear.SetPattern(s::bear_stance_key.GetValue()))
    {
        logs::error("bear stance key set failed");
    }

    if (!i->hotkey_hawk.SetPattern(s::hawk_stance_key.GetValue()))
    {
        logs::error("hawk stance key set failed");
    }

    if (!i->hotkey_wolf.SetPattern(s::wolf_stance_key.GetValue()))
    {
        logs::error("wolf stance key set failed");
    }
}

void STNG::InputEventListener::ProcessStanceKey(const KeyCombination* key)
{
    if (!key->IsTriggered())
        return;

    const auto stanceMan = GetSingleton();
    if (key == &stanceMan->hotkey_wolf)
    {
        if (StanceManager::CycleStancesPlayer())
            return;
    }

    for(auto& [hotkey, stance] : stanceMan->keySpellCombo)
    {
        if (key == hotkey)
        {
            StanceManager::UpdateStancePlayer(stance);
            return;
        }
    }
}

STNG::EventResult STNG::InputEventListener::ProcessEvent( RE::InputEvent* const* a_event,
                                                         RE::BSTEventSource<RE::InputEvent*>*)
{
    if (!a_event)
        return EventResult::kContinue;

    hotkey_neutral.Process(a_event, true);
    const bool wolfTriggered = hotkey_wolf.Process(a_event, true);
    hotkey_bear.Process(a_event, true);
    hotkey_hawk.Process(a_event, true);

    // Keep suppressing X until its release event has also passed through us.
    // This prevents Ready Weapon from firing on either press or release.
    static bool suppressGamepadXUntilRelease = false;

    const auto& wolfKeys = hotkey_wolf.GetKeys();
    const bool wolfIsLbX =
        wolfKeys.contains(kLeftBumperKey) &&
        wolfKeys.contains(kGamepadXKey);

    if (wolfTriggered && wolfIsLbX) {
        suppressGamepadXUntilRelease = true;
    }

    if (suppressGamepadXUntilRelease) {
        for (auto event = *a_event; event; event = event->next) {
            auto* button = event->AsButtonEvent();
            if (!button ||
                button->GetDevice() != RE::INPUT_DEVICE::kGamepad ||
                button->GetIDCode() != kGamepadXMask) {
                continue;
            }

            // Stances has already read the raw idCode. Clear only the mapped
            // vanilla action ("Ready Weapon"), leaving the physical X input intact.
            button->SetUserEvent(RE::BSFixedString{});

            if (button->IsUp()) {
                suppressGamepadXUntilRelease = false;
            }
        }
    }

    return EventResult::kContinue;
}
