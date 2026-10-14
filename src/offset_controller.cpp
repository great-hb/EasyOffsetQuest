#include "offset_controller.hpp"
#include "config.hpp"
#include "core/pose.hpp"
namespace OffsetController {
void Refresh() {}
void Apply(UnityEngine::XR::XRNode node,UnityEngine::Transform* transform) {
    if(!GetTweakConfig().enabled || !transform) return;
    auto& h=node==UnityEngine::XR::XRNode::LeftHand?GetTweakConfig().left:GetTweakConfig().right;
    auto p=transform->get_localPosition(); auto q=transform->get_localRotation();
    auto final=eoq::compose({{p.x,p.y,p.z},{q.x,q.y,q.z,q.w}},eoq::profileOffset({h.pivot.x,h.pivot.y,h.pivot.z},{h.rotation.x,h.rotation.y,h.rotation.z,h.rotation.w},h.zOffset));
    transform->set_localPosition({float(final.position.x),float(final.position.y),float(final.position.z)});
    transform->set_localRotation({float(final.rotation.x),float(final.rotation.y),float(final.rotation.z),float(final.rotation.w)});
}
}
