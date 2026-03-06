#pragma once

#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Vector3.h"

struct CameraForGPU {
    Matrix4x4 view;
    Matrix4x4 projection;
    Vector3 worldPosition;
private:
    float _pad = 0.0f;
};