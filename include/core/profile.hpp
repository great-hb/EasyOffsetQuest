#pragma once
#include "pose.hpp"
#include <cstdint>
#include <string>
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/prettywriter.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/stringbuffer.h"

namespace eoq {
struct Hand {
    Vec pivot{0,0,-.155};
    Vec euler{};
    double zOffset=.155;
    bool hasReference=false;
    Quat reference{};
    Pose offset() const { return profileOffset(pivot,unityEuler(euler),zOffset); }
};
struct Profile {
    std::int64_t unixTimestamp=0;
    std::string controllerType="Quest 2";
    Hand left{},right{};
};
inline const rapidjson::Value& required(const rapidjson::Value& obj,const char* name) {
    if(!obj.IsObject() || !obj.HasMember(name)) throw std::runtime_error(std::string("Missing ")+name);
    return obj[name];
}
inline double number(const rapidjson::Value& obj,const char* name) {
    auto& v=required(obj,name);
    if(!v.IsNumber() || !std::isfinite(v.GetDouble()) || std::abs(v.GetDouble())>100000)
        throw std::runtime_error(std::string("Invalid number: ")+name);
    return v.GetDouble();
}
inline Vec readVec(const rapidjson::Value& v) { return {number(v,"x"),number(v,"y"),number(v,"z")}; }
inline Hand readHand(const rapidjson::Value& doc,std::string prefix) {
    Hand h;
    h.pivot=readVec(required(doc,(prefix+"SaberPivotPosition").c_str()))*.01;
    h.euler=readVec(required(doc,(prefix+"SaberRotationEuler").c_str()));
    h.zOffset=number(doc,(prefix+"SaberZOffset").c_str())*.01;
    auto flag=prefix+"SaberHasReference", ref=prefix+"SaberReference";
    if(doc.HasMember(flag.c_str())) {
        if(!doc[flag.c_str()].IsBool()) throw std::runtime_error("Reference flag must be boolean");
        h.hasReference=doc[flag.c_str()].GetBool();
    }
    if(doc.HasMember(ref.c_str())) {
        auto& r=doc[ref.c_str()];
        // Preserve reference data exactly; validate its norm without rewriting it.
        h.reference={number(r,"x"),number(r,"y"),number(r,"z"),number(r,"w")};
        normalized(h.reference);
    } else if(h.hasReference) throw std::runtime_error("Missing reference quaternion");
    return h;
}
inline Profile parseProfile(const std::string& json) {
    rapidjson::Document doc; doc.Parse(json.c_str());
    if(doc.HasParseError() || !doc.IsObject()) throw std::runtime_error("Invalid profile JSON");
    auto& version=required(doc,"Version");
    if(!version.IsString() || std::string(version.GetString())!="2.0")
        throw std::runtime_error("Expected EasyOffset Version 2.0");
    Profile p; p.left=readHand(doc,"Left"); p.right=readHand(doc,"Right");
    const char* timestamp=doc.HasMember("UnixTimestamp")?"UnixTimestamp":"unixTimestamp";
    if(doc.HasMember(timestamp)) {
        if(!doc[timestamp].IsInt64()) throw std::runtime_error("Invalid UnixTimestamp");
        p.unixTimestamp=doc[timestamp].GetInt64();
    }
    const char* type=doc.HasMember("ControllerType")?"ControllerType":"controllerType";
    if(doc.HasMember(type)) {
        if(!doc[type].IsString()) throw std::runtime_error("Invalid ControllerType");
        p.controllerType=doc[type].GetString();
    }
    return p;
}
inline rapidjson::Value jsonVec(Vec v,rapidjson::Document::AllocatorType& a) {
    rapidjson::Value out(rapidjson::kObjectType);
    out.AddMember("x",v.x,a); out.AddMember("y",v.y,a); out.AddMember("z",v.z,a); return out;
}
inline void writeHand(rapidjson::Document& doc,Hand h,std::string prefix) {
    auto& a=doc.GetAllocator();
    auto add=[&](std::string key,rapidjson::Value v) {
        doc.AddMember(rapidjson::Value(key.c_str(),a),v,a);
    };
    add(prefix+"SaberPivotPosition",jsonVec(h.pivot*100,a));
    add(prefix+"SaberRotationEuler",jsonVec(h.euler,a));
    add(prefix+"SaberZOffset",rapidjson::Value(h.zOffset*100));
    add(prefix+"SaberHasReference",rapidjson::Value(h.hasReference));
    auto q=jsonVec({h.reference.x,h.reference.y,h.reference.z},a); q.AddMember("w",h.reference.w,a);
    add(prefix+"SaberReference",std::move(q));
}
inline std::string serializeProfile(Profile p) {
    rapidjson::Document doc; doc.SetObject(); auto& a=doc.GetAllocator();
    doc.AddMember("Version","2.0",a); doc.AddMember("UnixTimestamp",p.unixTimestamp,a);
    doc.AddMember("ControllerType",rapidjson::Value(p.controllerType.c_str(),a),a);
    writeHand(doc,p.left,"Left"); writeHand(doc,p.right,"Right");
    rapidjson::StringBuffer buf; rapidjson::PrettyWriter<rapidjson::StringBuffer> w(buf); doc.Accept(w);
    return std::string(buf.GetString())+"\n";
}
}
