#pragma once

#include "3D/ObjClass.h"
#include "math/Transform.h"
#include <memory>

class Camera;

class HeadLeft {
public:
  ~HeadLeft();

  void Initialize(Camera* camera, const Vector3& initialPos);
  void Update();
  void Draw();

  void SetPosition(const Vector3& pos);

  // Transformを一括設定する（回転やスケールも含めて上書き）
  void SetTransform(const Transform& transform);

private:
  std::unique_ptr<ObjClass> obj_ = nullptr;
  Vector3 basePosition_ = {};
  float timer_ = 0.0f;
};
