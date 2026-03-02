#include "HeadLeft.h"
#include "camera/Camera.h"
#include <cmath>

HeadLeft::~HeadLeft() {}

void HeadLeft::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
  obj_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 赤色に設定
}

void HeadLeft::Update() {
  if (obj_) {
    obj_->Update();
  }
}

void HeadLeft::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void HeadLeft::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
  if (obj_) {
    obj_->SetPosition(pos);
  }
}

void HeadLeft::SetTransform(const Transform& transform) {
  basePosition_ = transform.translate;
  if (obj_) {
    obj_->SetTransform(transform);
  }
}
