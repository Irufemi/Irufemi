#include "HeadLeft.h"
#include "camera/Camera.h"
#include <cmath>

HeadLeft::~HeadLeft() {}

void HeadLeft::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
}

void HeadLeft::Update() {
}

void HeadLeft::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void HeadLeft::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
}
