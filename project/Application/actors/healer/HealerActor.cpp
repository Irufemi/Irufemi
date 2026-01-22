#include "HealerActor.h"


#include "function/Math.h"

#include "Camera/Camera.h"
#include <cmath>


HealerActor::HealerActor() {}

HealerActor::~HealerActor() {}

void HealerActor::Initialize(Camera* camera, const Vector3& pos) {

	camera_ = camera;

	worldTransform_.translate = pos;
	worldTransform_.scale = { 0.3f, 0.3f, 0.3f };

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera_,"TD_Healer.obj");

}

void HealerActor::Update() {

}

void HealerActor::Draw() {

	model_->SetTransform(worldTransform_);
	model_->Update();
	model_->Draw();
}

Vector3 HealerActor::GetPosition() const {
	return worldTransform_.translate;
}

void HealerActor::MoveTowards(const Vector3& target, float speed) {
	Vector3& pos = worldTransform_.translate;
	Vector3 dir{ target.x - pos.x, target.y - pos.y, target.z - pos.z };
	float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	if (len < 1e-4f) return;
	dir.x /= len; dir.y /= len; dir.z /= len;
	pos.x += dir.x * speed;
	pos.y += dir.y * speed;
	pos.z += dir.z * speed;
}

void HealerActor::RefreshTransform() {

}

void HealerActor::SetAssigned(bool assigned) { assigned_ = assigned; }

bool HealerActor::IsAssigned() const { return assigned_; }
