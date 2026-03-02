#pragma once

#include "3D/ObjClass.h"
#include <memory>

class Camera;

class Body {
public:
  ~Body();

  void Initialize(Camera* camera, const Vector3& initialPos);
  void Update();
  void Draw();

  void SetPosition(const Vector3& pos);
  const Vector3& GetPosition() const;

private:
  std::unique_ptr<ObjClass> obj_ = nullptr;
  Vector3 basePosition_ = {};
};
