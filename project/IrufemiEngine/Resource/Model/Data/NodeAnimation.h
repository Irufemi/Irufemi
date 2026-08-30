#pragma once

#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector3.h"
#include "Resource/Model/Data/Keyframe.h"
#include <vector>

template <typename tValue> struct AnimationCurve {
    std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
    AnimationCurve<Irufemi::Vector3> translate;
    AnimationCurve<Irufemi::Quaternion> rotate;
    AnimationCurve<Irufemi::Vector3> scale;
};