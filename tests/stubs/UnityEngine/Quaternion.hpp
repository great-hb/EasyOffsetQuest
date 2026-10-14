#pragma once
#include "UnityEngine/Vector3.hpp"
// Host stand-in for Unity's float operators, not an engine runtime test.
namespace UnityEngine {
struct Quaternion {
    float x=0,y=0,z=0,w=1;
    static Quaternion op_Multiply(Quaternion a,Quaternion b) {
        return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
                a.w*b.y+a.y*b.w+a.z*b.x-a.x*b.z,
                a.w*b.z+a.z*b.w+a.x*b.y-a.y*b.x,
                a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
    }
    static Vector3 op_Multiply(Quaternion q,Vector3 p) {
        float x=q.x*2.f,y=q.y*2.f,z=q.z*2.f;
        float xx=q.x*x,yy=q.y*y,zz=q.z*z,xy=q.x*y,xz=q.x*z,yz=q.y*z,wx=q.w*x,wy=q.w*y,wz=q.w*z;
        return {(1.f-(yy+zz))*p.x+(xy-wz)*p.y+(xz+wy)*p.z,
                (xy+wz)*p.x+(1.f-(xx+zz))*p.y+(yz-wx)*p.z,
                (xz-wy)*p.x+(yz+wx)*p.y+(1.f-(xx+yy))*p.z};
    }
    static Quaternion Inverse(Quaternion q) {
        float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        return {-q.x/n,-q.y/n,-q.z/n,q.w/n};
    }
};
}
