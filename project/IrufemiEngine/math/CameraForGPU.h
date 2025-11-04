#pragma once

#include "math/Vector3.h"

struct CameraForGPU {
    Vector3 worldPosition;
    float _pad = 0.0f;
};