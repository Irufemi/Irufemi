#pragma once
#include "Engine/Core/Math/Matrix4x4.h"


namespace Irufemi {
struct PerView {
    Matrix4x4 viewProjection;
    Matrix4x4 billboardMatrix;
    Vector3 worldPosition;
    float cameraNear;
    float cameraFar;
    float pad[3];
};
} // namespace Irufemi
