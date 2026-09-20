#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"

namespace Irufemi {
struct QuaternionTransform {
    Vector3 scale{1.0f, 1.0f, 1.0f};
    Quaternion rotate{0.0f, 0.0f, 0.0f, 1.0f}; ///< 単位クォータニオン (w=1)
    Vector3 translate{0.0f, 0.0f, 0.0f};
};
} // namespace Irufemi
