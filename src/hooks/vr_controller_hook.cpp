#include "main.hpp"
#include "config.hpp"
#include "runtime_pose.hpp"
#include "gesture_controller.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Time.hpp"
MAKE_HOOK_MATCH(VRController_Update,&GlobalNamespace::VRController::Update,void,GlobalNamespace::VRController* self) {
    if(!self) return;
    auto node=self->_node;
    bool hand=node==UnityEngine::XR::XRNode::LeftHand || node==UnityEngine::XR::XRNode::RightHand;
    if(!hand || !GetTweakConfig().enabled || !self->_vrPlatformHelper || !self->_viewAnchorTransform) {
        if(hand && !GetTweakConfig().enabled) GestureController::Cancel();
        VRController_Update(self); return;
    }
    UnityEngine::Vector3 p=self->_lastTrackedPosition;
    UnityEngine::Quaternion q=self->_lastTrackedRotation;
    bool valid=self->_vrPlatformHelper->GetNodePose(node,self->_nodeIdx,byref(p),byref(q));
    self->_poseValid=valid; self->_poseFrame=UnityEngine::Time::get_frameCount();
    if(valid) { self->_lastTrackedPosition=p; self->_lastTrackedRotation=q; }
    else { p=self->_lastTrackedPosition; q=self->_lastTrackedRotation; }
    double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(n<1e-8) { VRController_Update(self); return; }
    auto basis=GetTweakConfig().trackingSpace==0?eoq::runtime::metaLinkBasis():eoq::runtime::Pose{};
    auto raw=eoq::runtime::compose({p,q},basis);
    GestureController::OnControllerUpdate(node,raw.position,raw.rotation,0);
    auto& h=node==UnityEngine::XR::XRNode::LeftHand?GetTweakConfig().left:GetTweakConfig().right;
    auto offset=eoq::runtime::profileOffset(h.pivot,h.rotation,h.zOffset);
    auto root=self->get_transform(); auto* anchor=self->_viewAnchorTransform.ptr();
    eoq::runtime::Pose relative;
    if(!eoq::runtime::relativeAnchor(root,anchor,relative)) { VRController_Update(self); return; }
    auto final=eoq::runtime::compose(eoq::runtime::compose(raw,offset),eoq::runtime::inverse(relative));
    root->SetLocalPositionAndRotation(final.position,final.rotation);
}
void InstallVRControllerHooks() { INSTALL_HOOK(PaperLogger,VRController_Update); }
