#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"

namespace Irufemi {

template <typename tValue> struct Keyframe {
    float time;
    tValue value;
};
using KeyframeVector3 = Keyframe<Irufemi::Vector3>;
using KeyframeQuaternion = Keyframe<Irufemi::Quaternion>;

} // namespace Irufemi

using Irufemi::Keyframe;
using Irufemi::KeyframeQuaternion;
using Irufemi::KeyframeVector3;