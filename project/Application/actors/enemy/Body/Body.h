#pragma once

#include "Irufemi.h"
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

  void SetHP(int hp) { hp_ = hp; }
  int GetHP() const { return hp_; }

  // Transformを一括設定する（回転やスケールも含めて上書き）
  void SetTransform(const Transform& transform);

private:
  std::unique_ptr<ObjClass> obj_ = nullptr;
  Vector3 basePosition_ = {};
  int hp_ = 0;
};
