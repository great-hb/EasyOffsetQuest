#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace eoq {
struct Vec { double x=0, y=0, z=0; };
struct Quat { double x=0, y=0, z=0, w=1; };
struct Pose { Vec position{}; Quat rotation{}; };
inline Vec operator+(Vec a, Vec b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec operator-(Vec a, Vec b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec operator*(Vec a, double s) { return {a.x*s,a.y*s,a.z*s}; }
inline Quat multiply(Quat a, Quat b) {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
            a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline Quat normalized(Quat q) {
    double n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if (!std::isfinite(n) || n<1e-8) throw std::runtime_error("Invalid quaternion");
    return {q.x/n,q.y/n,q.z/n,q.w/n};
}
inline Quat inverse(Quat q) { q=normalized(q); return {-q.x,-q.y,-q.z,q.w}; }
inline Vec rotate(Quat q, Vec v) {
    q=normalized(q);
    auto r=multiply(multiply(q,{v.x,v.y,v.z,0}),inverse(q));
    return {r.x,r.y,r.z};
}
inline Pose compose(Pose a, Pose b) {
    return {a.position+rotate(a.rotation,b.position),normalized(multiply(a.rotation,b.rotation))};
}
inline Pose inverse(Pose p) { auto q=inverse(p.rotation); return {rotate(q,p.position*-1),q}; }
// Unity Quaternion.Euler: apply Z, then X, then Y (qY * qX * qZ).
inline Quat unityEuler(Vec angles) {
    constexpr double halfRadians=3.14159265358979323846/360.;
    auto x=angles.x*halfRadians, y=angles.y*halfRadians, z=angles.z*halfRadians;
    return normalized(multiply(multiply({0,std::sin(y),0,std::cos(y)},
                                       {std::sin(x),0,0,std::cos(x)}),
                                       {0,0,std::sin(z),std::cos(z)}));
}
inline Vec unityEuler(Quat q) {
    q=normalized(q);
    auto xAxis=rotate(q,{1,0,0}), yAxis=rotate(q,{0,1,0}), zAxis=rotate(q,{0,0,1});
    double x=std::asin(std::clamp(-zAxis.y,-1.,1.)), y, z;
    if (std::abs(std::cos(x))>1e-7) {
        y=std::atan2(zAxis.x,zAxis.z); z=std::atan2(xAxis.y,yAxis.y);
    } else { y=std::atan2(-xAxis.z,xAxis.x); z=0; }
    constexpr double degrees=180./3.14159265358979323846;
    return {x*degrees,y*degrees,z*degrees};
}
// Stored IEEE-754 floats recovered from the Meta Touch grip definition,
// converted from OVR to Unity coordinates. Do not reconstruct via Euler(-60).
inline Pose metaLinkBasis() {
    return {{0,double(-.03f),double(-.04f)}, {-.5,0,0,double(0.8660253882408142f)}};
}
// PC EasyOffset parses centimetres as float, then multiplies by 0.01f.
inline float profileCentimetresToMetres(double centimetres) { return float(centimetres)*.01f; }
inline Pose profileOffset(Vec pivot, Quat rotation, double zOffset) {
    return {pivot+rotate(rotation,{0,0,zOffset}),rotation};
}
// The final view pose is rawNative * basis * EasyOffset. A is compensated
// at runtime, including its translation; stock offsets never accumulate.
inline Pose controllerPose(Pose rawNative, Pose basis, Pose offset, Pose anchor) {
    return compose(compose(compose(rawNative,basis),offset),inverse(anchor));
}
}
