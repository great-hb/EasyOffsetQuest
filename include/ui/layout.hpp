#pragma once
// BSML Lite clones full-width settings prefabs. Layout cells must resize both
// the prefab root and its inner widget; scaling the root leaves stale offsets.
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Text.hpp"
#include "bsml/shared/BSML/Components/ExternalComponents.hpp"
#include "bsml/shared/BSML/Components/ScrollView.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/ToggleSetting.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/LayoutGroup.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "TMPro/FontStyles.hpp"
#include "TMPro/TextOverflowModes.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

namespace OffsetUI {
inline UnityEngine::RectTransform* Rect(UnityEngine::GameObject* go) {
    return go->GetComponent<UnityEngine::RectTransform*>();
}
inline void Bounds(UnityEngine::GameObject* go,float width,float height) {
    // Reuse and update existing elements. Adding another LayoutElement lets
    // the prefab's larger preferred size continue to win layout calculations.
    auto elements=go->GetComponents<UnityEngine::UI::LayoutElement*>();
    if(elements.size()==0) go->AddComponent<UnityEngine::UI::LayoutElement*>();
    for(auto* e:go->GetComponents<UnityEngine::UI::LayoutElement*>()) {
        e->set_layoutPriority(100);
        e->set_minWidth(width<0?0:width); e->set_preferredWidth(width<0?0:width);
        e->set_flexibleWidth(width<0?1:0);
        e->set_minHeight(height); e->set_preferredHeight(height); e->set_flexibleHeight(0);
    }
    for(auto* fitter:go->GetComponents<UnityEngine::UI::ContentSizeFitter*>()) {
        fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
        fitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
    }
    go->get_transform()->set_localScale({1,1,1});
}
inline void Stretch(UnityEngine::RectTransform* rect,float left=0,float right=0) {
    if(!rect) return;
    rect->set_anchorMin({0,0}); rect->set_anchorMax({1,1});
    rect->set_pivot({.5f,.5f}); rect->set_offsetMin({left,0}); rect->set_offsetMax({-right,0});
    rect->set_localScale({1,1,1});
}
inline void Font(TMPro::TextMeshProUGUI* text,float size=3.2f,bool wrap=false) {
    if(!text) return;
    text->set_fontStyle(TMPro::FontStyles::Normal);
    text->set_enableAutoSizing(true); text->set_fontSize(size);
    text->set_fontSizeMax(size); text->set_fontSizeMin(size-.5f);
    text->set_enableWordWrapping(wrap); text->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    text->set_margin({0,0,0,0}); text->set_raycastTarget(false);
}
inline TMPro::TextMeshProUGUI* Text(UnityEngine::Transform* parent,StringW value,float height=6,float size=3.2f,bool wrap=false) {
    auto* text=BSML::Lite::CreateText(parent,value,TMPro::FontStyles::Normal,size);
    Bounds(text->get_gameObject(),-1,height); Font(text,size,wrap);
    text->set_alignment(TMPro::TextAlignmentOptions::Midline);
    return text;
}
inline UnityEngine::UI::HorizontalLayoutGroup* Row(UnityEngine::Transform* parent,float height,float spacing=1) {
    auto* row=BSML::Lite::CreateHorizontalLayoutGroup(parent);
    Bounds(row->get_gameObject(),-1,height);
    row->set_spacing(spacing); row->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
    row->set_childControlWidth(true); row->set_childControlHeight(true);
    row->set_childForceExpandWidth(false); row->set_childForceExpandHeight(false);
    return row;
}
inline UnityEngine::UI::VerticalLayoutGroup* Column(UnityEngine::Transform* parent,float height) {
    auto* column=BSML::Lite::CreateVerticalLayoutGroup(parent);
    Bounds(column->get_gameObject(),-1,height);
    column->set_spacing(.5f); column->set_childControlWidth(true); column->set_childControlHeight(true);
    column->set_childForceExpandWidth(true); column->set_childForceExpandHeight(false);
    column->set_childAlignment(UnityEngine::TextAnchor::UpperCenter);
    return column;
}
inline UnityEngine::GameObject* ScrollContent(UnityEngine::Transform* parent) {
    auto* content=BSML::Lite::CreateScrollView(parent);
    auto* external=content->GetComponent<BSML::ExternalComponents*>();
    auto* scroll=external->Get<BSML::ScrollView*>();
    auto* frame=external->Get<UnityEngine::RectTransform*>();
    Stretch(frame); frame->set_sizeDelta({-8,-4}); frame->set_anchoredPosition({-2,0});
    auto* layout=content->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>();
    layout->set_childControlWidth(true); layout->set_childControlHeight(true);
    layout->set_childForceExpandWidth(true); layout->set_childForceExpandHeight(false);
    layout->set_childAlignment(UnityEngine::TextAnchor::UpperCenter); layout->set_spacing(.5f);
    auto* contentRect=Rect(content);
    contentRect->set_anchorMin({0,1}); contentRect->set_anchorMax({1,1});
    contentRect->set_sizeDelta({0,0}); contentRect->set_pivot({.5f,1});
    // Width follows the viewport; only the vertical dimension fits content.
    auto outer=scroll->_contentRectTransform->get_gameObject();
    auto* outerLayout=outer->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>();
    outerLayout->set_childForceExpandWidth(true);
    auto* fitter=outer->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
    fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
    fitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    return content;
}
inline void Dropdown(BSML::DropdownListSetting* setting,bool compact=false) {
    auto* widget=setting->dropdown;
    auto root=widget->get_transform()->get_parent()->get_gameObject();
    Bounds(root,-1,6);
    auto label=root->get_transform()->Find("Label");
    if(compact) {
        if(label) label->get_gameObject()->SetActive(false);
        Stretch(Rect(widget->get_gameObject()),1,1);
    } else {
        if(label) { Stretch(label->GetComponent<UnityEngine::RectTransform*>(),0,53); Font(label->GetComponent<TMPro::TextMeshProUGUI*>()); }
        auto* rect=Rect(widget->get_gameObject());
        rect->set_anchorMin({1,0}); rect->set_anchorMax({1,1}); rect->set_pivot({1,.5f});
        rect->set_sizeDelta({50,0}); rect->set_anchoredPosition({0,0});
    }
    Font(widget->_text.ptr(),2.8f);
}
inline void Toggle(BSML::ToggleSetting* setting) {
    Bounds(setting->get_gameObject(),-1,6); Font(setting->text,3);
    constexpr float switchInset=2.5f;
    Stretch(Rect(setting->text->get_gameObject()),0,18+switchInset);
    auto* rect=Rect(setting->toggle->get_gameObject());
    rect->set_anchorMin({1,.5f}); rect->set_anchorMax({1,.5f}); rect->set_pivot({1,.5f});
    rect->set_anchoredPosition({-switchInset,0}); rect->set_sizeDelta({14,5});
}
inline void Button(UnityEngine::UI::Button* button,float width=-1) {
    Bounds(button->get_gameObject(),width,6);
    for(auto* text:button->GetComponentsInChildren<TMPro::TextMeshProUGUI*>(true)) Font(text,2.8f);
}
// BSML buttons clone PracticeButton. Its StackLayoutGroup normally resizes
// BG, Underline and Content together. After disabling that layout for a small
// arrow, every layer must be anchored explicitly or BG keeps its 24-unit width.
inline void StepButton(UnityEngine::UI::Button* button) {
    Bounds(button->get_gameObject(),5,5);
    for(auto* layout:button->GetComponentsInChildren<UnityEngine::UI::LayoutGroup*>(true)) layout->set_enabled(false);
    for(auto* fitter:button->GetComponentsInChildren<UnityEngine::UI::ContentSizeFitter*>(true)) fitter->set_enabled(false);
    for(auto name:{"BG","Underline","Content"}) {
        auto child=button->get_transform()->Find(name);
        if(!child) continue;
        auto* rect=child->GetComponent<UnityEngine::RectTransform*>();
        Stretch(rect);
        rect->set_offsetMin({0,.5f}); rect->set_offsetMax({0,-.5f});
    }
    for(auto* text:button->GetComponentsInChildren<TMPro::TextMeshProUGUI*>(true)) {
        Stretch(Rect(text->get_gameObject())); Font(text,3.2f);
        text->set_alignment(TMPro::TextAlignmentOptions::Center);
    }
}
inline void Slider(BSML::SliderSetting* setting) {
    Bounds(setting->get_gameObject(),-1,5);
    auto title=setting->get_transform()->Find("Title");
    if(title) title->get_gameObject()->SetActive(false);
    // SliderSetting::Setup assigns text to the value, not the title. A separate
    // readout beside the bar remains visible even when prefab text is hidden.
    auto* value=setting->slider->_valueText.ptr();
    if(value) value->get_gameObject()->SetActive(false);
    if(setting->decButton) setting->decButton->get_gameObject()->SetActive(false);
    if(setting->incButton) setting->incButton->get_gameObject()->SetActive(false);
    setting->slider->get_transform()->SetParent(setting->get_transform(),false);
    Stretch(Rect(setting->slider->get_gameObject()));
    auto background=setting->slider->get_transform()->Find("BG");
    if(background) {
        auto* rect=background->GetComponent<UnityEngine::RectTransform*>();
        Stretch(rect); rect->set_offsetMin({0,.5f}); rect->set_offsetMax({0,-.5f});
    }
    auto slidingArea=setting->slider->get_transform()->Find("SlidingArea");
    if(slidingArea) {
        auto* rect=slidingArea->GetComponent<UnityEngine::RectTransform*>();
        Stretch(rect); rect->set_offsetMin({1,1.5f}); rect->set_offsetMax({-1,-1.5f});
    }
}
}
