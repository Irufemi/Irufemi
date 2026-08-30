#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"

struct AnimationTrasform {
    Irufemi::Vector3 scale;
    Irufemi::Quaternion rotate;
    Irufemi::Vector3 translate;
};