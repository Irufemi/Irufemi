#pragma once

#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector3.h"

struct AnimationTrasform {
    Irufemi::Vector3 scale;
    Irufemi::Quaternion rotate;
    Irufemi::Vector3 translate;
};