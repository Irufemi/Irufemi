#pragma once

#include "math/Matrix4x4.h"
#include "math/Vector3.h"

struct CameraForGPU {
    Matrix4x4 view;
    Matrix4x4 projection;
    Vector3 worldPosition;
private:
    float _pad = 0.0f;
};