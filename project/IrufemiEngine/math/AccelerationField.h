#pragma once

#include "Vector3.h"
#include "shape/AABB.h"

struct AccelerationField {
    Vector3 acceleration; //!< 加速度
    AABB area; //!< 範囲
};