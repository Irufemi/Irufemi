#pragma once
#include <memory>
#include <array>
#include "actors/enemy/Body/Body.h"
#include "actors/enemy/Head/Left/HeadLeft.h"
#include "actors/enemy/Head/Mid/HeadMid.h"
#include "actors/enemy/Head/Right/HeadRight.h"
#include "EnemyAI.h"

class Camera;

class Enemy {

public:
  ~Enemy();

  void Initialize(Camera *camera);

  void Update();
  void Draw();

  // --- AIなど外部からTransformを操作するためのゲッター ---
  Transform& GetGlobalTransform() { return globalTransform_; }
  
  Transform& GetBodyLocalTransform(int index) { return bodyLocalTransforms_[index]; }
  Transform& GetHeadLeftLocalTransform() { return headLeftLocalTransform_; }
  Transform& GetHeadMidLocalTransform() { return headMidLocalTransform_; }
  Transform& GetHeadRightLocalTransform() { return headRightLocalTransform_; }
  // ---------------------------------------------------

private:
  std::array<std::unique_ptr<Body>, 3> bodies_;

  std::unique_ptr<HeadLeft> headLeft_ = nullptr;
  std::unique_ptr<HeadMid> headMid_ = nullptr;
  std::unique_ptr<HeadRight> headRight_ = nullptr;

  // 全体のTransform（親）
  Transform globalTransform_;

  // 各部位のローカルTransform（子）
  std::array<Transform, 3> bodyLocalTransforms_;
  Transform headLeftLocalTransform_;
  Transform headMidLocalTransform_;
  Transform headRightLocalTransform_;

  std::unique_ptr<EnemyAI> ai_ = nullptr;

  Camera* camera_ = nullptr;

  float fallSpeed_ = 0.05f;
  float shakeIntensity_ = 1.0f;
  bool isFalling_[4] = {false, false, false, false}; // 0-2: bodies, 3: head

  bool isActive_ = false;
};
