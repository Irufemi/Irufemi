#pragma once

#include "Core/Math/Matrix4x4.h"

struct WellForGPU {
    Irufemi::Matrix4x4 skeletonSpaceMatrix;                 // 位置用
    Irufemi::Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線用
};