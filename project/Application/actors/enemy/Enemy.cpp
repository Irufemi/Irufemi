#include "Enemy.h"
#include "camera/Camera.h"

Enemy::~Enemy() {}

void Enemy::Initialize(Camera *camera) {

  // ボディの初期化
  body_ = std::make_unique<ObjClass>();
  body_->Initialize(camera, "enemy/body.obj");
  isActivebody_ = true;
  // 頭の初期化
  head_ = std::make_unique<ObjClass>();
  head_->Initialize(camera, "enemy/head.obj");
  isActivehead_ = true;
}

void Enemy::Update() {
  if (isActivebody_) {
    body_->Update();
  }
  if (isActivehead_) {
    head_->Update();
  }
}

void Enemy::Draw() {
  if (isActivebody_) {
    body_->Draw();
  }
  if (isActivehead_) {
    head_->Draw();
  }
}
