#pragma once

#include <vector>
#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"
#include "Resource/Model/Data/Keyframe.h"

template<typename tValue>
struct AnimationCurve {
    std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
    AnimationCurve<Irufemi::Vector3> translate;
    AnimationCurve<Irufemi::Quaternion> rotate;
    AnimationCurve<Irufemi::Vector3> scale;
};