#include "HeadRight.h"
#include "camera/Camera.h"
#include <cmath>

HeadRight::~HeadRight() {}

void HeadRight::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/head.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
}

void HeadRight::Update() {
}

void HeadRight::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void HeadRight::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
}
