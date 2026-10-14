#include "main.hpp"

#include "config.hpp"
#include "i18n.hpp"
#include "offset_controller.hpp"
#include "scene_tracker.hpp"
#include "scotland2/shared/modloader.h"
#include "ui/settings_tab.hpp"
#include "bsml/shared/BSML.hpp"
#include "UnityEngine/Application.hpp"

void InstallVRControllerHooks();

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
// Stores the ID and version of our mod, and is sent to
// the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
// other config tools such as config-utils don't use this config, so it can be
// removed if those are in use
Configuration &getConfig() {
  static Configuration config(modInfo);
  return config;
}

// Called at the early stages of game loading
MOD_EXTERN_FUNC void setup(CModInfo *info) noexcept {
  *info = modInfo.to_c();

  // File logging
  Paper::Logger::RegisterFileContextId(PaperLogger.tag);

  // Load the JSON config now (no il2cpp calls), but defer the offset cache
  // refresh until late_load when il2cpp_functions::Init() has run -- otherwise
  // Quaternion::Euler / op_Multiply tries to look up Unity types too early.
  LoadTweakConfig();

}

// Called later on in the game loading - a good time to install function hooks
MOD_EXTERN_FUNC void late_load() noexcept {
  il2cpp_functions::Init();
  if (!std::string(UnityEngine::Application::get_version()).starts_with("1.40.8")) {
    PaperLogger.error("EasyOffsetQuest requires Beat Saber 1.40.8; hooks were not installed");
    return;
  }
  BSML::Init();
  InitializeRuntimeConfig();

  OffsetController::Refresh();


  InstallVRControllerHooks();
  SceneTracker::InstallHooks();


  SetLanguage(static_cast<Language>(GetTweakConfig().language));
  RegisterSettingsTab();
}
