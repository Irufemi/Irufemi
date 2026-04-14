#pragma once

#include "../../Core/Math/Matrix4x4.h"
#include "../../Core/Math/Vector3.h"

struct CameraForGPU {
  Matrix4x4 view = {};
  Matrix4x4 projection = {};
  Vector3 worldPosition = {};
  float time = 0.0f;
  float deltaTime = 0.0f;
  float _pad[2] = { 0.0f, 0.0f }; // 16byte align
};
