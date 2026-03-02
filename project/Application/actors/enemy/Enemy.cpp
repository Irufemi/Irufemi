#include "Enemy.h"
#include "camera/Camera.h"
#include "Body.h"
#include "HeadLeft.h"
#include "HeadMid.h"
#include "HeadRight.h"

Enemy::~Enemy() {}

void Enemy::Initialize(Camera *camera) {
  // ボディの初期化 (3段)
  for (int i = 0; i < 3; ++i) {
    bodies_[i] = std::make_unique<Body>();
    // だるま落としのように縦に積む。間隔は仮で2.0fとする
    bodies_[i]->Initialize(camera, Vector3{-0.5f, i * 2.0f, 0.0f});
  }

  // 頭の初期化 (ボディの最も上の行の、さらに上)
  float topY = 2 * 2.0f + 2.0f; // 3段目がY=4.0、その上
  
  headLeft_ = std::make_unique<HeadLeft>();
  headLeft_->Initialize(camera, Vector3{-2.5f, topY, 0.0f});

  headMid_ = std::make_unique<HeadMid>();
  headMid_->Initialize(camera, Vector3{-0.5f, topY, 0.0f});

  headRight_ = std::make_unique<HeadRight>();
  headRight_->Initialize(camera, Vector3{1.5f, topY, 0.0f});

  isActive_ = true;
}

void Enemy::Update() {
  if (!isActive_) return;

  for (auto& body : bodies_) {
    if (body) body->Update();
  }
  if (headLeft_) headLeft_->Update();
  if (headMid_) headMid_->Update();
  if (headRight_) headRight_->Update();
}

void Enemy::Draw() {
  if (!isActive_) return;

  for (auto& body : bodies_) {
    if (body) body->Draw();
  }
  if (headLeft_) headLeft_->Draw();
  if (headMid_) headMid_->Draw();
  if (headRight_) headRight_->Draw();
}
