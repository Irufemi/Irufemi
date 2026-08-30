#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"

namespace Irufemi {
struct QuaternionTransform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;
};
} // namespace Irufemi
