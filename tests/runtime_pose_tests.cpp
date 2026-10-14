#include "runtime_pose.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace eoq::runtime;
UnityEngine::Quaternion q(eoq::Vec e) {
    auto v=eoq::unityEuler(e);return {float(v.x),float(v.y),float(v.z),float(v.w)};
}
void emit(Pose p) {
    for(float v:{p.position.x,p.position.y,p.position.z,p.rotation.x,p.rotation.y,p.rotation.z,p.rotation.w}) std::cout<<v<<',';
}
int main() {
    std::cout<<std::setprecision(17);
    for(int i=0;i<2000;i++) {
        UnityEngine::Transform root{{100000.f,-100000.f,100000.f},{1,1,1},q({20,40,60}),nullptr};
        UnityEngine::Transform middle{{.003f,-.02f,.006f},{1,1,1},q({double(i%79-39),23,-15}),&root};
        UnityEngine::Transform anchor{{-.005f,.03f,.055f},{1,1,1},q({-40,double(i%61-30),11}),&middle};
        Pose relative;
        if(!relativeAnchor(&root,&anchor,relative)) throw std::runtime_error("Valid nested hierarchy rejected");
        Pose raw{{.1f,.9f,-.4f},q({double(i%179-89),double(i*7%359-179),double(i*11%359-179)})};
        Pose off=profileOffset({-.02f,.006f,-.04f},q({double(i*3%179-89),double(i*5%359-179),double(i*13%359-179)}),.155f);
        auto final=compose(compose(compose(raw,metaLinkBasis()),off),inverse(relative));
        emit(raw);emit(off);emit({middle.position,middle.rotation});emit({anchor.position,anchor.rotation});emit(relative);emit(final);
        std::cout<<'\n';
        root.position={0,0,0}; root.rotation={0,0,0,1};
        Pose nearOrigin;
        if(!relativeAnchor(&root,&anchor,nearOrigin)) throw std::runtime_error("Root moved unexpectedly changed ancestry");
        if(relative.position.x!=nearOrigin.position.x || relative.position.y!=nearOrigin.position.y || relative.position.z!=nearOrigin.position.z ||
           relative.rotation.x!=nearOrigin.rotation.x || relative.rotation.y!=nearOrigin.rotation.y || relative.rotation.z!=nearOrigin.rotation.z || relative.rotation.w!=nearOrigin.rotation.w)
            throw std::runtime_error("Anchor compensation depends on root world pose");
    }
    UnityEngine::Transform root,anchor; Pose p;
    if(relativeAnchor(&root,&anchor,p)) throw std::runtime_error("Unrelated anchor accepted");
    anchor.parent=&root; anchor.scale={1,1.1f,1};
    if(relativeAnchor(&root,&anchor,p)) throw std::runtime_error("Scaled anchor accepted as rigid");
    if(!relativeAnchor(&root,&root,p)) throw std::runtime_error("Root anchor rejected");
}
