#include "Body.h"
#include "camera/Camera.h"

Body::~Body() {}

void Body::Initialize(Camera* camera, const Vector3& initialPos) {
  obj_ = std::make_unique<ObjClass>();
  obj_->Initialize(camera, "enemy/body.obj");
  basePosition_ = initialPos;
  obj_->SetPosition(basePosition_);
}

void Body::Update() {
  if (obj_) {
    obj_->Update();
  }
}

void Body::Draw() {
  if (obj_) {
    obj_->Draw();
  }
}

void Body::SetPosition(const Vector3& pos) {
  basePosition_ = pos;
  if (obj_) {
    obj_->SetPosition(pos);
  }
}

void Body::SetTransform(const Transform& transform) {
  basePosition_ = transform.translate;
  if (obj_) {
    obj_->SetTransform(transform);
  }
}

const Vector3& Body::GetPosition() const {
  return basePosition_;
}
