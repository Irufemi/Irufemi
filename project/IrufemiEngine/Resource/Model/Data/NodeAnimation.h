#pragma once

#include <vector>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Keyframe.h"

template<typename tValue>
struct AnimationCurve {
    std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
    AnimationCurve<Irufemi::Vector3> translate;
    AnimationCurve<Irufemi::Quaternion> rotate;
    AnimationCurve<Irufemi::Vector3> scale;
};