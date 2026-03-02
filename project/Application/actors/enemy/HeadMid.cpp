#include "HeadMid.h"
#include "camera/Camera.h"
#include <cmath>

HeadMid::~HeadMid() {}

void HeadMid::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
}

void HeadMid::Update() {
}

void HeadMid::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void HeadMid::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
}
