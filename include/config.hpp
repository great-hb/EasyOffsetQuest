#pragma once
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Vector3.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
enum class AdjustmentMode : int { None=0, Position=1, Rotation=2, PosAndRot=3, Direct=4, PositionAuto=5, RotationAuto=6 };
enum class ControllerButton : int { Trigger=0, Grip=1, PrimaryButton=2, SecondaryButton=3, ThumbstickPress=4 };
constexpr std::string_view AdjustmentModeToString(AdjustmentMode m) {
    switch(m) {
    case AdjustmentMode::Position: return "Position";
    case AdjustmentMode::Rotation: return "Rotation";
    case AdjustmentMode::PosAndRot: return "Basic";
    case AdjustmentMode::Direct: return "Direct";
    case AdjustmentMode::PositionAuto: return "Position Auto";
    case AdjustmentMode::RotationAuto: return "Rotation Auto";
    default: return "None";
    }
}
struct HandTweakConfig {
    UnityEngine::Vector3 pivot{0,0,-.155f};
    float zOffset=.155f;
    UnityEngine::Quaternion rotation{0,0,0,1};
    UnityEngine::Vector3 rotationEuler{0,0,0};
    bool hasReference=false;
    UnityEngine::Quaternion reference{0,0,0,1};
};
struct OffsetConfig {
    bool enabled=true;
    HandTweakConfig left{},right{};
    AdjustmentMode adjustmentMode=AdjustmentMode::None;
    ControllerButton assignedButton=ControllerButton::ThumbstickPress;
    bool useFreeHand=true;
    bool uiLock=false;
    int language=1;
    int trackingSpace=0; // PC Meta Link grip (0), native Quest helper (1).
    std::string controllerType="Quest 2";
    std::int64_t unixTimestamp=0;
    std::string activePreset="Default";
};
inline constexpr float kSaberFwdX=0, kSaberFwdY=0, kSaberFwdZ=1;
OffsetConfig& GetTweakConfig();
void LoadTweakConfig();
void InitializeRuntimeConfig();
bool SaveTweakConfig();
std::vector<std::string> ListPresets();
bool SavePreset(const std::string& name);
bool LoadPreset(const std::string& name);
std::string CreatePreset();
const std::string& PresetStatus();
void SyncQuatFromEuler(HandTweakConfig& cfg);
void SyncEulerFromQuat(HandTweakConfig& cfg);
