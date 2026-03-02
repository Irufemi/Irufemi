#pragma once

#include "3D/ObjClass.h"
#include <memory>

class Camera;

class HeadRight {
public:
  ~HeadRight();

  void Initialize(Camera *camera, const Vector3 &initialPos);
  void Update();
  void Draw();

  void SetPosition(const Vector3 &pos);

private:
  std::unique_ptr<ObjClass> obj_ = nullptr;
  Vector3 basePosition_;
  float timer_ = 0.0f;
};
