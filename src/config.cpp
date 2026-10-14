#include "main.hpp"
#include "config.hpp"
#include "core/profile.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace {
OffsetConfig g_config;
std::string g_status="Ready";
bool g_activeWritable=true;
bool g_runtimeReady=false;
std::filesystem::path DataDir() { return getDataDir(getConfig().info); }
std::filesystem::path PresetsDir() { return DataDir()/"Presets"; }
bool ValidName(const std::string& name) {
    return !name.empty() && name.size()<=100 && name.front()!='.' && name.front()!='_' &&
           name.find_first_of("/\\:*?\"<>|\r\n")==std::string::npos;
}
bool AtomicWrite(const std::filesystem::path& path,const std::string& text) {
    std::error_code ec; std::filesystem::create_directories(path.parent_path(),ec);
    if(ec) { g_status="Cannot create preset directory"; return false; }
    auto temp=path; temp += ".tmp";
    FILE* f=std::fopen(temp.c_str(),"wb");
    if(!f) { g_status="Cannot write preset"; return false; }
    bool ok=std::fwrite(text.data(),1,text.size(),f)==text.size();
    if(std::fflush(f)!=0) ok=false;
    if(::fsync(fileno(f))!=0) ok=false;
    if(std::fclose(f)!=0) ok=false;
    if(!ok) { g_status="Preset write failed"; return false; }
    if(std::filesystem::exists(path)) {
        auto backup=path; backup += ".bak";
        std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing,ec);
        if(ec) { g_status="Cannot preserve previous preset"; return false; }
    }
    if(std::rename(temp.c_str(),path.c_str())!=0) { g_status="Cannot replace preset"; return false; }
    return true;
}
eoq::Hand ToCore(const HandTweakConfig& h) {
    return {{h.pivot.x,h.pivot.y,h.pivot.z},{h.rotationEuler.x,h.rotationEuler.y,h.rotationEuler.z},
             h.zOffset,h.hasReference,{h.reference.x,h.reference.y,h.reference.z,h.reference.w}};
}
HandTweakConfig FromCore(const eoq::Hand& h,const rapidjson::Value& document,const std::string& prefix) {
    HandTweakConfig out;
    auto& pivot=document[(prefix+"SaberPivotPosition").c_str()];
    out.pivot={eoq::profileCentimetresToMetres(pivot["x"].GetDouble()),
               eoq::profileCentimetresToMetres(pivot["y"].GetDouble()),
               eoq::profileCentimetresToMetres(pivot["z"].GetDouble())};
    out.rotationEuler={float(h.euler.x),float(h.euler.y),float(h.euler.z)};
    out.zOffset=eoq::profileCentimetresToMetres(document[(prefix+"SaberZOffset").c_str()].GetDouble()); out.hasReference=h.hasReference;
    out.reference={float(h.reference.x),float(h.reference.y),float(h.reference.z),float(h.reference.w)};
    SyncQuatFromEuler(out); return out;
}
std::string ProfileText() {
    return eoq::serializeProfile({g_config.unixTimestamp,g_config.controllerType,ToCore(g_config.left),ToCore(g_config.right)});
}
bool ReadProfile(const std::filesystem::path& path) {
    try {
        if(std::filesystem::file_size(path)>512*1024) throw std::runtime_error("Profile is too large");
        std::ifstream file(path); if(!file) throw std::runtime_error("Cannot open profile");
        std::ostringstream text; text<<file.rdbuf();
        auto json=text.str(); auto profile=eoq::parseProfile(json);
        rapidjson::Document document; document.Parse(json.c_str());
        auto left=FromCore(profile.left,document,"Left"),right=FromCore(profile.right,document,"Right");
        g_config.left=left; g_config.right=right;
        g_config.controllerType=profile.controllerType; g_config.unixTimestamp=profile.unixTimestamp;
        return true;
    } catch(const std::exception& e) { g_status=e.what(); PaperLogger.warn("Profile load failed: {}",g_status); return false; }
}
}
OffsetConfig& GetTweakConfig() { return g_config; }
const std::string& PresetStatus() { return g_status; }
void SyncQuatFromEuler(HandTweakConfig& h) {
    if(g_runtimeReady) h.rotation=UnityEngine::Quaternion::Euler(h.rotationEuler);
    else {
        // setup() runs before IL2CPP is initialized. Recompute with Unity in
        // InitializeRuntimeConfig() before installing the controller hook.
        auto q=eoq::unityEuler(eoq::Vec{h.rotationEuler.x,h.rotationEuler.y,h.rotationEuler.z});
        h.rotation={float(q.x),float(q.y),float(q.z),float(q.w)};
    }
}
void InitializeRuntimeConfig() {
    g_runtimeReady=true;
    SyncQuatFromEuler(g_config.left); SyncQuatFromEuler(g_config.right);
}
void SyncEulerFromQuat(HandTweakConfig& h) {
    auto v=eoq::unityEuler(eoq::Quat{h.rotation.x,h.rotation.y,h.rotation.z,h.rotation.w});
    h.rotationEuler={float(v.x),float(v.y),float(v.z)};
}
void LoadTweakConfig() {
    getConfig().Load(); auto& doc=getConfig().config;
    auto integer=[&](const char* key,int fallback) { return doc.IsObject() && doc.HasMember(key) && doc[key].IsInt()?doc[key].GetInt():fallback; };
    auto boolean=[&](const char* key,bool fallback) { return doc.IsObject() && doc.HasMember(key) && doc[key].IsBool()?doc[key].GetBool():fallback; };
    g_config.enabled=boolean("Enabled",true);
    g_config.useFreeHand=boolean("UseFreeHand",true); g_config.uiLock=boolean("UILock",false);
    g_config.language=std::clamp(integer("Language",1),0,1);
    g_config.trackingSpace=std::clamp(integer("TrackingSpace",0),0,1);
    g_config.assignedButton=static_cast<ControllerButton>(std::clamp(integer("AssignedButton",4),0,4));
    g_config.adjustmentMode=AdjustmentMode::None;
    if(doc.IsObject() && doc.HasMember("ActivePreset") && doc["ActivePreset"].IsString() && ValidName(doc["ActivePreset"].GetString()))
        g_config.activePreset=doc["ActivePreset"].GetString();
    std::error_code ec; std::filesystem::create_directories(PresetsDir(),ec);
    auto active=DataDir()/"_active.json";
    if(std::filesystem::exists(active) && !ReadProfile(active)) {
        g_activeWritable=false; g_config.enabled=false;
        g_status="Active profile invalid; load a valid preset";
    }
    if(!std::filesystem::exists(PresetsDir()/"Default.json")) AtomicWrite(PresetsDir()/"Default.json",ProfileText());
}
bool SaveTweakConfig() {
    auto& doc=getConfig().config; doc.SetObject(); auto& a=doc.GetAllocator();
    doc.AddMember("Enabled",g_config.enabled,a); doc.AddMember("UseFreeHand",g_config.useFreeHand,a);
    doc.AddMember("UILock",g_config.uiLock,a); doc.AddMember("Language",g_config.language,a);
    doc.AddMember("TrackingSpace",g_config.trackingSpace,a);
    doc.AddMember("AssignedButton",int(g_config.assignedButton),a);
    doc.AddMember("ActivePreset",rapidjson::Value(g_config.activePreset.c_str(),a),a);
    getConfig().Write();
    return g_activeWritable && AtomicWrite(DataDir()/"_active.json",ProfileText());
}
std::vector<std::string> ListPresets() {
    std::vector<std::string> names; std::error_code ec;
    for(auto const& entry:std::filesystem::directory_iterator(PresetsDir(),ec))
        if(entry.is_regular_file() && entry.path().extension()==".json" && ValidName(entry.path().stem().string())) names.push_back(entry.path().stem().string());
    std::sort(names.begin(),names.end()); if(names.empty()) names.push_back("Default"); return names;
}
bool SavePreset(const std::string& name) {
    if(!ValidName(name)) { g_status="Invalid preset name"; return false; }
    g_config.unixTimestamp=std::time(nullptr);
    if(!AtomicWrite(PresetsDir()/(name+".json"),ProfileText())) return false;
    g_config.activePreset=name; g_activeWritable=true;
    bool ok=SaveTweakConfig(); if(ok) g_status="Saved "+name; return ok;
}
bool LoadPreset(const std::string& name) {
    if(!ValidName(name)) { g_status="Invalid preset name"; return false; }
    if(!ReadProfile(PresetsDir()/(name+".json"))) return false;
    g_config.activePreset=name; g_activeWritable=true;
    bool ok=SaveTweakConfig(); if(ok) g_status="Loaded "+name; return ok;
}
std::string CreatePreset() {
    for(int i=1;i<10000;i++) {
        auto name="Preset "+std::to_string(i);
        if(!std::filesystem::exists(PresetsDir()/(name+".json"))) return SavePreset(name)?name:"";
    }
    g_status="Too many presets"; return "";
}
