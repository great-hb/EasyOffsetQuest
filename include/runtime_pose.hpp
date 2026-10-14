#pragma once
#include "core/pose.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"

namespace eoq::runtime {
struct Pose {
    UnityEngine::Vector3 position{0,0,0};
    UnityEngine::Quaternion rotation{0,0,0,1};
};
inline UnityEngine::Vector3 add(UnityEngine::Vector3 a,UnityEngine::Vector3 b) {
    return {a.x+b.x,a.y+b.y,a.z+b.z};
}
inline Pose compose(Pose a,Pose b) {
    return {add(a.position,UnityEngine::Quaternion::op_Multiply(a.rotation,b.position)),
            UnityEngine::Quaternion::op_Multiply(a.rotation,b.rotation)};
}
inline Pose inverse(Pose p) {
    auto q=UnityEngine::Quaternion::Inverse(p.rotation);
    return {UnityEngine::Quaternion::op_Multiply(q,UnityEngine::Vector3{-p.position.x,-p.position.y,-p.position.z}),q};
}
inline Pose metaLinkBasis() {
    auto b=eoq::metaLinkBasis();
    return {{float(b.position.x),float(b.position.y),float(b.position.z)},
            {float(b.rotation.x),float(b.rotation.y),float(b.rotation.z),float(b.rotation.w)}};
}
inline Pose profileOffset(UnityEngine::Vector3 pivot,UnityEngine::Quaternion rotation,float zOffset) {
    return {add(pivot,UnityEngine::Quaternion::op_Multiply(rotation,UnityEngine::Vector3{0,0,zOffset})),rotation};
}
// Read the rigid controller-to-anchor chain in local space. Unlike subtracting
// two world poses, this does not lose precision as the player moves from origin.
inline bool relativeAnchor(UnityEngine::Transform* root,UnityEngine::Transform* anchor,Pose& result) {
    result={};
    auto current=anchor;
    for(int depth=0;current && current!=root && depth<64;depth++) {
        auto scale=current->get_localScale();
        if(scale.x!=1.f || scale.y!=1.f || scale.z!=1.f) return false;
        result=compose({current->get_localPosition(),current->get_localRotation()},result);
        current=current->get_parent().ptr();
    }
    if(current!=root) return false;
    auto scale=root->get_localScale();
    return scale.x==1.f && scale.y==1.f && scale.z==1.f;
}
}
