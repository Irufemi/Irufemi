#pragma once

#include "../../../Engine/Core/Math/Matrix4x4.h"
#include "../../../Engine/Core/Math/QuaternionTransform.h"
#include <string>
#include <vector>

struct Node {
    Irufemi::QuaternionTransform transform;
    Irufemi::Matrix4x4 localMatrix;
    std::string name;
    std::vector<Node> children;
};