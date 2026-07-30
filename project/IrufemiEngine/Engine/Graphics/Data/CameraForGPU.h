#pragma once

#include "../../Core/Math/Matrix4x4.h"
#include "../../Core/Math/Vector3.h"

struct CameraForGPU {
  Irufemi::Matrix4x4 view = {};
  Irufemi::Matrix4x4 projection = {};
  Irufemi::Vector3 worldPosition = {};
};
