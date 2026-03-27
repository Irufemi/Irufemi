#pragma once

#include "Engine/Core/Math/Matrix4x4.h"
#include "Resource/Model/Data/VertexWeightData.h"
#include <vector>

struct JointWeightData {
    Matrix4x4 inverseBindPoseMatrix;
    std::vector<VertexWeightData> vertexWeights;
};