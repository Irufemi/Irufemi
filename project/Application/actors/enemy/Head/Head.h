#pragma once
#include "Engine/Core/Math/Transform.h"
#include <memory>
#include "Engine/Core/Math/Geometry/OBB.h"
#include "Engine/Core/Math/Vector4.h"

class Camera;
class IrufemiEngine;
class VoxelParticleSystem;
class ObjClass;
class CylinderClass;
class ParticleSystem;

class Head {
public:
  Head();
  virtual ~Head();

  void Initialize(const Vector3& initialPos);
  void Update();
  void Draw(IrufemiEngine* engine);

  void SetPosition(const Vector3& pos);

  bool ApplyDamage(int damage);

  void SetHP(int hp) { hp_ = hp; }
  int GetHP() const { return hp_; }

  void SetPhase2(bool isPhase2) { isPhase2_ = isPhase2; }

  // Transformを一括設定する（回転やスケールも含めて上書き）
  void SetTransform(const Transform& transform, const Vector3* drawWorldPos = nullptr);
  const Transform& GetTransform() const { return transform_; }
  const Vector3& GetDrawPosition() const { return drawPosition_; }

  void OnDestroyed(const Vector3& attackDir, float blowSpeed);
  bool IsCompletelyDead() const;
  bool IsBlownAway() const { return isBlownAway_; }
  float GetBlowTimer() const { return blowTimer_; }

  const Vector3& GetBlowVelocity() const { return blowVelocity_; }
  void SetBlowVelocity(const Vector3& v) { blowVelocity_ = v; }

  OBB GetOBB() const;

  // 指定した位置でパーティクルをはじけさせる
  void ScatterAt(const Vector3& velocity, const OBB& collisionArea);

protected:
  Vector4 baseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

private:
  std::unique_ptr<ObjClass> obj_ = nullptr;
  Vector3 basePosition_ = {};
  Transform transform_ = {}; // OBB計算や吹き飛び時の姿勢保持用
  Vector3 drawPosition_ = {}; // アニメーション等のオフセットを含んだ描画位置
  float timer_ = 0.0f; // 既存のタイマー
  int hp_ = 0;

  float damageFlashTimer_ = 0.0f;

  // 吹き飛び・消滅用
  bool isBlownAway_ = false;
  float blowTimer_ = 0.0f;
  Vector3 blowVelocity_ = {};
  float disappearTimer_ = 0.0f;

  std::unique_ptr<VoxelParticleSystem> voxelSystem_ = nullptr;
  std::shared_ptr<CylinderClass> thrusterFlame_ = nullptr;

  // パーティクルシステム
  std::unique_ptr<ParticleSystem> flameParticle_ = nullptr;
  std::unique_ptr<ParticleSystem> smokeParticle_ = nullptr;

  bool isPhase2_ = false;
};
