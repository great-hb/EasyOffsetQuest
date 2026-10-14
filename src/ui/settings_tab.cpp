#include "ui/settings_tab.hpp"
#include "config.hpp"
#include "gesture_controller.hpp"
#include "main.hpp"
#include "ui/layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Buttons.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Settings.hpp"
#include "bsml/shared/BSML-Lite/Creation/Text.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/ToggleSetting.hpp"
#include "bsml/shared/BSML/GameplaySetup/GameplaySetup.hpp"
#include "bsml/shared/BSML/Tags/ModalKeyboardTag.hpp"
#include "bsml/shared/BSML/Components/Keyboard/ModalKeyboard.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace {
UnityW<UnityEngine::GameObject> g_container;
std::array<std::array<UnityW<BSML::SliderSetting>,7>,2> g_sliders{};
std::array<std::array<UnityW<TMPro::TextMeshProUGUI>,7>,2> g_values{};
std::array<std::array<std::array<UnityW<UnityEngine::UI::Button>,2>,7>,2> g_stepButtons{};
UnityW<TMPro::TextMeshProUGUI> g_status;
UnityW<BSML::DropdownListSetting> g_presetDropdown;
UnityW<BSML::DropdownListSetting> g_modeDropdown,g_buttonDropdown,g_typeDropdown,g_spaceDropdown;
UnityW<BSML::ToggleSetting> g_freeHand,g_enabled;
UnityW<UnityEngine::UI::Button> g_saveAs;
UnityW<HMUI::InputFieldView> g_nameField;
UnityW<BSML::ModalKeyboard> g_nameKeyboard;
// Expose the same factory used by BSML's string-setting tag for Lite UI.
struct PresetKeyboardTag : BSML::ModalKeyboardTag {
    using BSML::ModalKeyboardTag::CreateObject;
};
std::string g_newPresetName="New Preset";
std::vector<std::string> g_presetNames;
std::vector<std::string_view> g_presetViews;
std::string g_selectedPreset="Default";
bool g_syncing=false;
std::array<std::string_view,7> modes={"None","Basic","Position","Rotation","Direct","Position Auto","Rotation Auto"};
constexpr std::array<AdjustmentMode,7> modeValues={AdjustmentMode::None,AdjustmentMode::PosAndRot,AdjustmentMode::Position,AdjustmentMode::Rotation,AdjustmentMode::Direct,AdjustmentMode::PositionAuto,AdjustmentMode::RotationAuto};
std::array<std::string_view,5> buttons={"Thumbstick","Trigger","Grip","A / X","B / Y"};
constexpr std::array<ControllerButton,5> buttonValues={ControllerButton::ThumbstickPress,ControllerButton::Trigger,ControllerButton::Grip,ControllerButton::PrimaryButton,ControllerButton::SecondaryButton};
std::array<std::string_view,3> types={"Quest 2","Quest 3","Quest Pro"};
std::array<std::string_view,2> spaces={"PC Meta Link","Native Quest"};
HandTweakConfig& Hand(int index) { return index==0?GetTweakConfig().left:GetTweakConfig().right; }
float Value(const HandTweakConfig& h,int field) {
    switch(field) {
    case 0:return h.zOffset*100; case 1:return h.pivot.x*100; case 2:return h.pivot.y*100; case 3:return h.pivot.z*100;
    case 4:return h.rotationEuler.x; case 5:return h.rotationEuler.y; default:return h.rotationEuler.z;
    }
}
float Minimum(int field) { return field==0?0.f:field<4?-40.f:-180.f; }
float Maximum(int field) { return field==0?30.f:field<4?40.f:180.f; }
void Readout(int hand,int field) {
    if(!g_values[hand][field]) return;
    char text[32];
    std::snprintf(text,sizeof(text),"%.2f",Value(Hand(hand),field));
    g_values[hand][field]->set_text(StringW(text));
}
void SetField(int hand,int field,float value) {
    if(g_syncing) return;
    if(GetTweakConfig().uiLock) { RefreshSettingsValues(); return; }
    GestureController::Cancel(); auto& h=Hand(hand);
    switch(field) {
    case 0:h.zOffset=value*.01f;break; case 1:h.pivot.x=value*.01f;break;
    case 2:h.pivot.y=value*.01f;break; case 3:h.pivot.z=value*.01f;break;
    case 4:h.rotationEuler.x=value;break;case 5:h.rotationEuler.y=value;break;case 6:h.rotationEuler.z=value;break;
    }
    if(field>=4) SyncQuatFromEuler(h);
    SaveTweakConfig(); Readout(hand,field);
}
void Step(int hand,int field,int direction) {
    if(g_syncing || GetTweakConfig().uiLock) return;
    const float value=std::clamp(Value(Hand(hand),field)+direction*.1f,Minimum(field),Maximum(field));
    SetField(hand,field,value);
    // Programmatic slider values may be quantized by HMUI. Keep the readout
    // tied to the actual profile and suppress callbacks while syncing the bar.
    RefreshSettingsValues();
}
void Status() { if(g_status) g_status->set_text(StringW(PresetStatus())); }
void Mirror(int from) {
    if(GetTweakConfig().uiLock) return;
    GestureController::Cancel(); auto h=Hand(from);
    h.pivot.x=-h.pivot.x; h.rotationEuler.y=-h.rotationEuler.y; h.rotationEuler.z=-h.rotationEuler.z;
    h.reference.y=-h.reference.y; h.reference.z=-h.reference.z;
    SyncQuatFromEuler(h); Hand(1-from)=h; SaveTweakConfig(); RefreshSettingsValues();
}
void Reset(int hand) {
    if(GetTweakConfig().uiLock) return;
    GestureController::Cancel(); Hand(hand)=HandTweakConfig{}; SaveTweakConfig(); RefreshSettingsValues();
}
void Build(UnityEngine::GameObject* container,bool first) {
    g_container=container;
    if(!first) { RefreshSettingsValues(); return; }
    // Prefab setup can emit value changes; preserve imported profile precision.
    g_syncing=true;
    auto* content=OffsetUI::ScrollContent(container->get_transform());
    auto vt=content->get_transform();
    // Same three-column options row as PC EasyOffset's TopPanel.
    auto* options=OffsetUI::Row(vt,10);
    for(int column=0;column<3;column++) {
        auto* group=OffsetUI::Column(options->get_transform(),10);
        auto p=group->get_transform();
        OffsetUI::Text(p,column==0?"Controller type":column==1?"Adjustment mode":"Button",3.5f,2.8f);
        if(column==0) g_typeDropdown=BSML::Lite::CreateDropdown(p,"",GetTweakConfig().controllerType,std::span<std::string_view>(types),[](StringW value) {
            if(g_syncing || GetTweakConfig().uiLock) return;
            GetTweakConfig().controllerType=std::string(value); SaveTweakConfig();
        });
        if(column==1) g_modeDropdown=BSML::Lite::CreateDropdown(p,"",AdjustmentModeToString(GetTweakConfig().adjustmentMode),std::span<std::string_view>(modes),[](StringW value) {
            if(g_syncing || GetTweakConfig().uiLock) return;
            GestureController::Cancel();
            for(size_t i=0;i<modes.size();i++) if(std::string(value)==modes[i]) GetTweakConfig().adjustmentMode=modeValues[i];
        });
        if(column==2) g_buttonDropdown=BSML::Lite::CreateDropdown(p,"","Thumbstick",std::span<std::string_view>(buttons),[](StringW value) {
            if(g_syncing || GetTweakConfig().uiLock) return;
            GestureController::Cancel();
            for(size_t i=0;i<buttons.size();i++) if(std::string(value)==buttons[i]) GetTweakConfig().assignedButton=buttonValues[i];
            SaveTweakConfig();
        });
        OffsetUI::Dropdown(column==0?g_typeDropdown.ptr():column==1?g_modeDropdown.ptr():g_buttonDropdown.ptr(),true);
    }
    g_spaceDropdown=BSML::Lite::CreateDropdown(vt,"Profile space",spaces[GetTweakConfig().trackingSpace],std::span<std::string_view>(spaces),[](StringW value) {
        if(g_syncing || GetTweakConfig().uiLock) return;
        GestureController::Cancel();
        GetTweakConfig().trackingSpace=std::string(value)==spaces[0]?0:1;
        SaveTweakConfig(); RefreshSettingsValues();
    });
    OffsetUI::Dropdown(g_spaceDropdown.ptr());
    auto* toggles=OffsetUI::Row(vt,6,2);
    auto* enabledCell=OffsetUI::Column(toggles->get_transform(),6);
    g_enabled=BSML::Lite::CreateToggle(enabledCell->get_transform(),"Enabled",GetTweakConfig().enabled,[](bool value) {
        if(g_syncing) return;
        GestureController::Cancel(); GetTweakConfig().enabled=value;
        SaveTweakConfig(); RefreshSettingsValues();
    });
    OffsetUI::Toggle(g_enabled.ptr());
    auto* freeHandCell=OffsetUI::Column(toggles->get_transform(),6);
    g_freeHand=BSML::Lite::CreateToggle(freeHandCell->get_transform(),"Free hand",GetTweakConfig().useFreeHand,[](bool value) {
        if(g_syncing || GetTweakConfig().uiLock) return;
        GestureController::Cancel(); GetTweakConfig().useFreeHand=value; SaveTweakConfig();
    });
    OffsetUI::Toggle(g_freeHand.ptr());
    auto* lockCell=OffsetUI::Column(toggles->get_transform(),6);
    auto* lock=BSML::Lite::CreateToggle(lockCell->get_transform(),"UI Lock",GetTweakConfig().uiLock,[](bool value) {
        if(g_syncing) return; GestureController::Cancel(); GetTweakConfig().uiLock=value; SaveTweakConfig(); RefreshSettingsValues();
    });
    OffsetUI::Toggle(lock);
    auto* hands=OffsetUI::Row(vt,4,1);
    auto* left=OffsetUI::Text(hands->get_transform(),"<color=#ff6666>Left</color>",4,3);
    left->set_alignment(TMPro::TextAlignmentOptions::Center);
    auto* units=OffsetUI::Text(hands->get_transform(),"cm / °",4,2.7f);
    OffsetUI::Bounds(units->get_gameObject(),14,4); units->set_alignment(TMPro::TextAlignmentOptions::Center);
    auto* right=OffsetUI::Text(hands->get_transform(),"<color=#7777ff>Right</color>",4,3);
    right->set_alignment(TMPro::TextAlignmentOptions::Center);
    constexpr std::array<const char*,7> labels={"Z Offset","Pivot X","Pivot Y","Pivot Z","Rot X","Rot Y","Rot Z"};
    // PC LabeledSlidersRow: left slider, centered axis label, right slider.
    for(int field=0;field<7;field++) {
        auto* row=OffsetUI::Row(vt,5,1);
        for(int hand=0;hand<2;hand++) {
            if(hand==1) {
                auto* label=OffsetUI::Text(row->get_transform(),labels[field],5,2.8f);
                OffsetUI::Bounds(label->get_gameObject(),14,5); label->set_alignment(TMPro::TextAlignmentOptions::Center);
            }
            auto* cell=OffsetUI::Row(row->get_transform(),5,.5f);
            auto parent=cell->get_transform();
            auto* decrease=BSML::Lite::CreateUIButton(parent,"‹",[hand,field]{Step(hand,field,-1);});
            OffsetUI::StepButton(decrease); g_stepButtons[hand][field][0]=decrease;
            auto* slider=BSML::Lite::CreateSliderSetting(parent,"",.1f,Value(Hand(hand),field),
                Minimum(field),Maximum(field),.12f,false,{0,0},[hand,field](float value) { SetField(hand,field,value); });
            OffsetUI::Slider(slider); g_sliders[hand][field]=slider;
            auto* increase=BSML::Lite::CreateUIButton(parent,"›",[hand,field]{Step(hand,field,1);});
            OffsetUI::StepButton(increase); g_stepButtons[hand][field][1]=increase;
            auto* readout=OffsetUI::Text(parent,"",5,3.0f);
            OffsetUI::Bounds(readout->get_gameObject(),13,5);
            readout->set_alignment(TMPro::TextAlignmentOptions::Center);
            g_values[hand][field]=readout; Readout(hand,field);
        }
    }
    auto* actions=OffsetUI::Row(vt,6);
    OffsetUI::Button(BSML::Lite::CreateUIButton(actions->get_transform(),"Reset L",[]{Reset(0);}));
    OffsetUI::Button(BSML::Lite::CreateUIButton(actions->get_transform(),"Mirror L → R",[]{Mirror(0);}));
    OffsetUI::Button(BSML::Lite::CreateUIButton(actions->get_transform(),"Mirror R → L",[]{Mirror(1);}));
    OffsetUI::Button(BSML::Lite::CreateUIButton(actions->get_transform(),"Reset R",[]{Reset(1);}));
    g_presetNames=ListPresets(); g_presetViews.clear();
    for(auto& name:g_presetNames) g_presetViews.push_back(name);
    g_selectedPreset=GetTweakConfig().activePreset;
    auto* bottom=OffsetUI::Row(vt,6);
    auto* presetCell=OffsetUI::Column(bottom->get_transform(),6);
    g_presetDropdown=BSML::Lite::CreateDropdown(presetCell->get_transform(),"",g_selectedPreset,std::span<std::string_view>(g_presetViews),[](StringW value) {
        if(!g_syncing) g_selectedPreset=std::string(value);
    });
    OffsetUI::Dropdown(g_presetDropdown.ptr(),true);
    OffsetUI::Button(BSML::Lite::CreateUIButton(bottom->get_transform(),"Save",[] {
        if(GetTweakConfig().uiLock) return; GestureController::Cancel(); SavePreset(g_selectedPreset); Status();
    }),14);
    OffsetUI::Button(BSML::Lite::CreateUIButton(bottom->get_transform(),"Load",[] {
        if(GetTweakConfig().uiLock) return; GestureController::Cancel();
        LoadPreset(g_selectedPreset); RefreshSettingsValues(); Status();
    }),14);
    OffsetUI::Button(BSML::Lite::CreateUIButton(bottom->get_transform(),"Refresh",[] { RebuildSettingsTab(); }),18);
    auto* newPreset=OffsetUI::Row(vt,7);
    auto* nameCell=OffsetUI::Column(newPreset->get_transform(),7);
    auto* name=BSML::Lite::CreateStringSetting(nameCell->get_transform(),"Preset name",g_newPresetName,[](StringW value) {
        if(!g_syncing) g_newPresetName=std::string(value);
    });
    OffsetUI::Bounds(name->get_gameObject(),-1,7);
    for(auto* text:name->GetComponentsInChildren<TMPro::TextMeshProUGUI*>(true)) OffsetUI::Font(text,2.8f);
    g_nameField=name;
    name->_useGlobalKeyboard=false;
    // Keep the keyboard outside ScrollContent. ModalView promotes it to the
    // view controller's popup layer and centers it independently of scrolling.
    auto* keyboardObject=PresetKeyboardTag{}.CreateObject(container->get_transform());
    keyboardObject->get_transform()->set_localScale({.75f,.75f,.75f});
    g_nameKeyboard=keyboardObject->GetComponent<BSML::ModalKeyboard*>();
    g_nameKeyboard->clearOnOpen=false;
    g_nameKeyboard->modalView->moveToCenter=true;
    g_nameKeyboard->modalView->dismissOnBlockerClicked=true;
    g_nameKeyboard->onEnter=[](StringW value) {
        if(GetTweakConfig().uiLock) return;
        g_newPresetName=std::string(value);
        if(g_nameField) g_nameField->SetText(value);
    };
    name->add_selectionStateDidChangeEvent(BSML::MakeSystemAction<HMUI::InputFieldView::SelectionState>(
        std::function<void(HMUI::InputFieldView::SelectionState)>([](auto state) {
            if(state!=HMUI::InputFieldView::SelectionState::Pressed || GetTweakConfig().uiLock || !g_nameKeyboard) return;
            g_nameKeyboard->modalView->Show();
            g_nameKeyboard->SetText(StringW(g_newPresetName));
        })));
    g_saveAs=BSML::Lite::CreateUIButton(newPreset->get_transform(),"Save As",[] {
        if(GetTweakConfig().uiLock) return;
        GestureController::Cancel();
        if(SavePreset(g_newPresetName)) RebuildSettingsTab();
        else Status();
    });
    OffsetUI::Button(g_saveAs.ptr(),18);
    g_status=OffsetUI::Text(vt,PresetStatus(),4,2.6f);
    RefreshSettingsValues();
}
}
void RefreshSettingsValues() {
    g_syncing=true;
    for(int hand=0;hand<2;hand++) for(int field=0;field<7;field++) {
        if(g_sliders[hand][field]) {
            g_sliders[hand][field]->set_Value(Value(Hand(hand),field));
            g_sliders[hand][field]->set_interactable(!GetTweakConfig().uiLock);
        }
        Readout(hand,field);
        for(auto& button:g_stepButtons[hand][field]) if(button) button->set_interactable(!GetTweakConfig().uiLock);
    }
    if(g_typeDropdown) { g_typeDropdown->set_Value(reinterpret_cast<System::Object*>(static_cast<Il2CppString*>(StringW(GetTweakConfig().controllerType)))); g_typeDropdown->set_interactable(!GetTweakConfig().uiLock); }
    if(g_modeDropdown) { g_modeDropdown->set_Value(reinterpret_cast<System::Object*>(static_cast<Il2CppString*>(StringW(AdjustmentModeToString(GetTweakConfig().adjustmentMode))))); g_modeDropdown->set_interactable(!GetTweakConfig().uiLock); }
    if(g_buttonDropdown) {
        for(size_t i=0;i<buttonValues.size();i++) if(buttonValues[i]==GetTweakConfig().assignedButton)
            g_buttonDropdown->set_Value(reinterpret_cast<System::Object*>(static_cast<Il2CppString*>(StringW(buttons[i]))));
        g_buttonDropdown->set_interactable(!GetTweakConfig().uiLock);
    }
    if(g_spaceDropdown) {
        g_spaceDropdown->set_Value(reinterpret_cast<System::Object*>(static_cast<Il2CppString*>(StringW(spaces[GetTweakConfig().trackingSpace]))));
        g_spaceDropdown->set_interactable(!GetTweakConfig().uiLock);
    }
    if(g_enabled) g_enabled->set_Value(GetTweakConfig().enabled);
    if(g_freeHand) {
        g_freeHand->set_Value(GetTweakConfig().useFreeHand);
        g_freeHand->set_interactable(!GetTweakConfig().uiLock);
    }
    if(g_nameField) g_nameField->set_interactable(!GetTweakConfig().uiLock);
    if(g_saveAs) g_saveAs->set_interactable(!GetTweakConfig().uiLock);
    if(g_presetDropdown) g_presetDropdown->set_interactable(!GetTweakConfig().uiLock);
    Status(); g_syncing=false;
}
void RegisterSettingsTab() {
    BSML::GameplaySetup::get_instance()->AddTab(std::function<void(UnityEngine::GameObject*,bool)>(&Build),"Easy Offset",BSML::MenuType::All);
}
void RebuildSettingsTab() {
    if(!g_container) return;
    GestureController::Cancel();
    // A shown modal is temporarily reparented outside the tab by HMUI.
    if(g_nameKeyboard) {
        g_nameKeyboard->modalView->HMUI::ModalView::Hide(false,nullptr);
        auto keyboardObject=g_nameKeyboard->get_gameObject();
        keyboardObject->SetActive(false); UnityEngine::Object::Destroy(keyboardObject);
        g_nameKeyboard=nullptr;
    }
    g_nameField=nullptr;
    auto parent=g_container->get_transform();
    for(int i=parent->get_childCount()-1;i>=0;i--) {
        auto child=parent->GetChild(i)->get_gameObject();
        child->SetActive(false); UnityEngine::Object::Destroy(child);
    }
    g_sliders={}; g_values={}; g_stepButtons={}; g_status=nullptr; g_presetDropdown=nullptr; g_modeDropdown=nullptr; g_buttonDropdown=nullptr; g_typeDropdown=nullptr; g_spaceDropdown=nullptr; g_freeHand=nullptr; g_enabled=nullptr; g_saveAs=nullptr;
    Build(g_container.ptr(),true);
}
void SyncDropdownsToConfig() {} // Dropdown callbacks commit immediately.
bool IsAdjustmentPanelVisible() { return g_container && g_container->get_activeInHierarchy(); }
