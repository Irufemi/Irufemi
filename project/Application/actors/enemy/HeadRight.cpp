#include "HeadRight.h"
#include "camera/Camera.h"
#include <cmath>

HeadRight::~HeadRight() {}

void HeadRight::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
  obj_->SetColor({0.0f, 0.0f, 1.0f, 1.0f}); // 青色に設定
}

void HeadRight::Update() {
  if (obj_) {
    obj_->Update();
  }
}

void HeadRight::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void HeadRight::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
}
