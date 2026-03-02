#include "HeadMid.h"
#include "camera/Camera.h"
#include <cmath>

HeadMid::~HeadMid() {}

void HeadMid::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
  obj_->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // 緑色に設定
}

void HeadMid::Update() {
  if (obj_) {
    obj_->Update();
  }
}

void HeadMid::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void HeadMid::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
  if (obj_) {
    obj_->SetPosition(pos);
  }
}

void HeadMid::SetTransform(const Transform& transform) {
  basePosition_ = transform.translate;
  if (obj_) {
    obj_->SetTransform(transform);
  }
}
