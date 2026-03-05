#pragma once

#include "Matrix4x4.h"
#include "VertexWeightData.h"
#include <vector>

struct JointWeightData {
    Matrix4x4 inverseBndPoseMatrix;
    std::vector<VertexWeightData> vertexWeights;
};