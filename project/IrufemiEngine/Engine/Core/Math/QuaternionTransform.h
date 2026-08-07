#pragma once

#include "Vector3.h"
#include "Quaternion.h"


namespace Irufemi {
struct QuaternionTransform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;
};
} // namespace Irufemi
