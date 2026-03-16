#pragma once

#include "Irufemi.h"
#include <memory>
#include "Engine/Core/Math/Geometry/OBB.h"

class Camera;

class Body {
public:
  ~Body();

  void Initialize(Camera* camera, const Vector3& initialPos);
  void Update();
  void Draw();

  void SetPosition(const Vector3& pos);
  const Vector3& GetPosition() const;

  void ApplyDamage(int damage);

  void SetHP(int hp) { hp_ = hp; }
  int GetHP() const { return hp_; }

  // Transformを一括設定する（回転やスケールも含めて上書き）
  void SetTransform(const Transform& transform, const Vector3* drawWorldPos = nullptr);
  const Transform& GetTransform() const { return transform_; }

  void OnDestroyed(const Vector3& attackDir, float blowSpeed);
  bool IsCompletelyDead() const;
  bool IsBlownAway() const { return isBlownAway_; }

  const Vector3& GetBlowVelocity() const { return blowVelocity_; }
  void SetBlowVelocity(const Vector3& v) { blowVelocity_ = v; }

  OBB GetOBB() const;

private:
  std::unique_ptr<ObjClass> obj_ = nullptr;
  Vector3 basePosition_ = {};
  Transform transform_ = {}; // OBB計算や吹き飛び時の姿勢保持のために保存

  int hp_ = 0;

  Vector4 baseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
  float damageFlashTimer_ = 0.0f;

  // 吹き飛び・消滅用
  bool isBlownAway_ = false;
  Vector3 blowVelocity_ = {};
  float disappearTimer_ = 0.0f;
};
