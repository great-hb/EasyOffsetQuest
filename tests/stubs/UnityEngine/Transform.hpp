#pragma once
#include "UnityEngine/Quaternion.hpp"
namespace UnityEngine {
struct Transform;
struct TransformRef { Transform* value; Transform* ptr() { return value; } };
struct Transform {
    Vector3 position{},scale{1,1,1}; Quaternion rotation{}; Transform* parent=nullptr;
    Vector3 get_localPosition() { return position; }
    Quaternion get_localRotation() { return rotation; }
    Vector3 get_localScale() { return scale; }
    TransformRef get_parent() { return {parent}; }
};
}
