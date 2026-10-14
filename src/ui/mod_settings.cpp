#include "ui/mod_settings.hpp"
#include "ui/settings_tab.hpp"
#include "config.hpp"
#include "gesture_controller.hpp"
#include "main.hpp"
#include "ui/layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Buttons.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Settings.hpp"
#include "bsml/shared/BSML-Lite/Creation/Text.hpp"
#include "bsml/shared/BSML/Settings/BSMLSettings.hpp"
#include "HMUI/ViewController.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include <array>
namespace {
std::string g_name="New Preset";
UnityW<TMPro::TextMeshProUGUI> g_status;
std::array<std::string_view,2> spaces={"PC Meta Link","Native Quest"};
void Build(HMUI::ViewController* vc,bool first,bool,bool) {
    if(!first) { if(g_status) g_status->set_text(StringW(PresetStatus())); return; }
    auto* content=OffsetUI::ScrollContent(vc->get_transform(),true); auto p=content->get_transform();
    OffsetUI::Text(p,"Adjust offsets: song selection → Easy Offset",6,3);
    OffsetUI::Text(p,"Mod Settings",4,3.3f);
    auto* enabled=BSML::Lite::CreateToggle(p,"Enabled",GetTweakConfig().enabled,[](bool v) {
        GestureController::Cancel(); GetTweakConfig().enabled=v; SaveTweakConfig();
    });
    OffsetUI::Toggle(enabled);
    // Put the label above a full-width dropdown, rather than sharing a
    // fixed native row with a long value and a long setting name.
    OffsetUI::Text(p,"Profile tracking space",3.5f,3);
    auto* tracking=BSML::Lite::CreateDropdown(p,"",spaces[GetTweakConfig().trackingSpace],std::span<std::string_view>(spaces),[](StringW v) {
        if(GetTweakConfig().uiLock) return;
        GestureController::Cancel(); GetTweakConfig().trackingSpace=std::string(v)==spaces[0]?0:1; SaveTweakConfig();
    });
    OffsetUI::Dropdown(tracking,true);
    OffsetUI::Text(p,"PC profiles: PC Meta Link",4,2.8f);
    OffsetUI::Text(p,"Presets",4,3.3f);
    auto* name=BSML::Lite::CreateStringSetting(p,"Preset name",g_name,[](StringW value) { g_name=std::string(value); });
    OffsetUI::Bounds(name->get_gameObject(),-1,7);
    for(auto* text:name->GetComponentsInChildren<TMPro::TextMeshProUGUI*>(true)) OffsetUI::Font(text,3);
    OffsetUI::Button(BSML::Lite::CreateUIButton(p,"Save As",[] {
        if(GetTweakConfig().uiLock) return; GestureController::Cancel();
        SavePreset(g_name); RebuildSettingsTab(); if(g_status) g_status->set_text(StringW(PresetStatus()));
    }));
    g_status=OffsetUI::Text(p,PresetStatus(),4,2.8f);
}
}
void RegisterModSettings() {
    BSML::BSMLSettings::get_instance()->TryAddSettingsMenu(std::function<void(HMUI::ViewController*,bool,bool,bool)>(&Build),"Easy Offset Quest");
}
