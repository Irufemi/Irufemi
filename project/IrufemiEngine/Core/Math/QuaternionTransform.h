#pragma once

#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector3.h"

namespace Irufemi {
struct QuaternionTransform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;
};
} // namespace Irufemi
