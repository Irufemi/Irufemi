#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"

template <typename tValue>
struct Keyframe {
    float time;
    tValue value;
};
using KeyframeVector3 = Keyframe<Irufemi::Vector3>;
using KeyframeQuaternion = Keyframe<Irufemi::Quaternion>;