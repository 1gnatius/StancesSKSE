#include "ingame-menu.h"

#include "formloader.h"
#include "inputmanager.h"
#include "Settings.h"

namespace STNG
{
    void RegisterFUCKMenu()
    {
        if (FUCK::Connect()) {
            SKSE::log::info("Connected to FUCK API");
            FUCK::RegisterTool(&g_stanceTool);
            SKSE::log::info("StancesNG tool registered");
        } else {
            SKSE::log::error("Failed to connect to FUCK API");
        }
    }

    void StanceTool::OnClose()
    {
        HotkeyOnSave();
        Config::Settings::UpdateSettings(true);
    }

    void StanceTool::OnOpen()
    {
        use_cycling = Config::Settings::use_cycling.GetValue();
        apply_on_start = Config::Settings::apply_stance_on_start.GetValue();
        play_animation = Config::Settings::play_transition_animation.GetValue();

        HotkeyOnLoad();
        RebuildFilteredIdles();

    }

    bool StanceTool::OnAsyncInput(const void* p)
    {
        if (FUCK::UpdateManagedHotkey(p, _bearKey)) return true;
        if (FUCK::UpdateManagedHotkey(p, _wolfKey)) return true;
        if (FUCK::UpdateManagedHotkey(p, _hawkKey)) return true;
        if (FUCK::UpdateManagedHotkey(p, _neutralKey)) return true;
        return false;
    }

    void StanceTool::DrawHotkeySelector()
    {
        FUCK::DrawManagedHotkey("Bear Stance", _bearKey);
        UnbindHotkey("Unbind##bear", _bearKey);
        FUCK::DrawManagedHotkey("Wolf Stance", _wolfKey);
        UnbindHotkey("Unbind##wolf", _wolfKey);
        FUCK::DrawManagedHotkey("Hawk Stance", _hawkKey);
        UnbindHotkey("Unbind##hawk",_hawkKey);
        FUCK::DrawManagedHotkey("Neutral Stance", _neutralKey);
        UnbindHotkey("Unbind##neutral",_neutralKey);
    }

    void StanceTool::DrawBoolManipulation()
    {

            if (FUCK::Checkbox("Use Cycling", &use_cycling, false, false))
            {
                Config::Settings::use_cycling.SetValue(use_cycling);
            }
            FUCK::SameLine();
            if (FUCK::Checkbox("Apply Stance on Start", &apply_on_start, false, false))
            {
                Config::Settings::apply_stance_on_start.SetValue(apply_on_start);
            }

            if (FUCK::Checkbox("Play Animation", &play_animation, false, false))
            {
                Config::Settings::play_transition_animation.SetValue(play_animation);
            }



    }

    void StanceTool::DrawIdleSelection()
    {
        if (FUCK::InputText("Filter##idle", &_currentFilter))
            RebuildFilteredIdles();

        FUCK::SameLine();
        if (FUCK::Button("Clear##idle")) {
            _selectedIdle = nullptr;
            Config::Settings::transition_animation_form.SetValue("");
        }


        FUCK::BeginChild("IdleList", ImVec2(0, 300.f), true, 0);

        constexpr int kMaxDisplay = 50;
        int displayed = 0;

        for (auto* form : _filteredIdles) {
            if (!form) continue;
            if (displayed >= kMaxDisplay) {
                FUCK::TextDisabled("Too many results - refine your filter...");
                break;
            }
            const bool selected = (form == _selectedIdle);
            if (FUCK::Selectable(form->GetFormEditorID(), selected)) {
                _selectedIdle = form;
                FormLoader::transition_animation = form;
                Config::Settings::transition_animation_form.SetValue(form->GetFormEditorID());
            }
            ++displayed;
        }

        FUCK::EndChild();

    }

    void StanceTool::RestoreDefaultSettings()
    {
        using s = Config::Settings;
        use_cycling = false;
        s::use_cycling.SetValue(false);
        apply_on_start = false;
        s::apply_stance_on_start.SetValue(false);
        play_animation = false;
        s::play_transition_animation.SetValue(false);
        _bearKey = KeyStringToManagedHotkey("shift+x");
        s::bear_stance_key.SetValue("shift+x");
        _wolfKey = KeyStringToManagedHotkey("x");
        s::wolf_stance_key.SetValue("x");
        _hawkKey = KeyStringToManagedHotkey("control+x");
        s::hawk_stance_key.SetValue("control+x");
        _neutralKey = KeyStringToManagedHotkey("alt+v");
        s::neutral_stance_key.SetValue("alt+v");
        _selectedIdle = nullptr;
        s::transition_animation_form.SetValue("");
    }

    void StanceTool::RebuildFilteredIdles()
    {
        _filteredIdles.clear();

        if (_currentFilter.empty()) {
            _filteredIdles = FormLoader::all_game_idles;
            return;
        }

        const auto lower = clib_util::string::tolower(_currentFilter);
        for (auto* form : FormLoader::all_game_idles) {
            if (!form) continue;
            const auto edid = string::tolower(std::string(form->GetFormEditorID()));
            if (edid.contains(lower))
                _filteredIdles.push_back(form);
        }
    }

    std::string StanceTool::ManagedHotkeyToKeyString(const FUCK::ManagedHotkey& a_hotkey)
    {
        if (a_hotkey.kKey == 0) return "";

        std::vector<std::string> parts;
        parts.emplace_back(details::GetNameByKey(a_hotkey.kKey));
        if (a_hotkey.kMod1 > 0) parts.emplace_back(details::GetNameByKey(static_cast<std::uint32_t>(a_hotkey.kMod1)));
        if (a_hotkey.kMod2 > 0) parts.emplace_back(details::GetNameByKey(static_cast<std::uint32_t>(a_hotkey.kMod2)));

        return string::join(parts, "+");
    }

    void StanceTool::UnbindHotkey(const char* label ,FUCK::ManagedHotkey &a_hotkey)
    {
        FUCK::SameLine();
        if (FUCK::Button(label))
        {
            a_hotkey.Clear();
        }
    }
    void StanceTool::HotkeyOnLoad()
    {
        _bearKey = KeyStringToManagedHotkey(Config::Settings::bear_stance_key.GetValue());
        _wolfKey = KeyStringToManagedHotkey(Config::Settings::wolf_stance_key.GetValue());
        _hawkKey = KeyStringToManagedHotkey(Config::Settings::hawk_stance_key.GetValue());
        _neutralKey = KeyStringToManagedHotkey(Config::Settings::neutral_stance_key.GetValue());
    }

    void StanceTool::HotkeyOnSave()
    {
        Config::Settings::bear_stance_key.SetValue(ManagedHotkeyToKeyString(_bearKey));
        Config::Settings::wolf_stance_key.SetValue(ManagedHotkeyToKeyString(_wolfKey));
        Config::Settings::hawk_stance_key.SetValue(ManagedHotkeyToKeyString(_hawkKey));
        Config::Settings::neutral_stance_key.SetValue(ManagedHotkeyToKeyString(_neutralKey));
    }

    FUCK::ManagedHotkey StanceTool::KeyStringToManagedHotkey(std::string_view a_str)
    {
        FUCK::ManagedHotkey hotkey;
        if (a_str.empty())
            return hotkey;

        std::string str = string::tolower(a_str);
        string::replace_all(str, " ", "");
        string::replace_all(str, "num+", "numplus");

        const auto parts = string::split(str, "+");
        try {
            std::vector<std::uint32_t> keys;
            for (const auto& part : parts)
                keys.push_back(details::GetKeyByName(part));

            if (keys.size() >= 1) hotkey.kKey  = keys[0];
            if (keys.size() >= 2) hotkey.kMod1 = static_cast<std::int32_t>(keys[1]);
            if (keys.size() >= 3) hotkey.kMod2 = static_cast<std::int32_t>(keys[2]);
        } catch (...){ }

        return hotkey;
    }
}
