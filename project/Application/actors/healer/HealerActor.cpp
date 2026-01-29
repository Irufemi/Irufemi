#include "HealerActor.h"


#include "function/Math.h"

#include "Camera/Camera.h"
#include <cmath>


HealerActor::HealerActor() {}

HealerActor::~HealerActor() {}

void HealerActor::Initialize(Camera* camera, const Vector3& pos) {

	camera_ = camera;

	transform_.translate = pos;
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	targetPosition_ = pos;

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera_,"TD_Healer.obj");

}

void HealerActor::Update() {
	if (!alive_)
	{
		return;
	}
	UpdateOBB();
}

void HealerActor::Draw() {
	if (!alive_)
	{
		return;
	}
	model_->SetTransform(transform_);
	model_->Update();
	model_->Draw();
}

Vector3 HealerActor::GetPosition() const {
	return transform_.translate;
}

void HealerActor::MoveTowards(const Vector3& target, float speed) {
	Vector3& pos = transform_.translate;
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

void HealerActor::SetAlive(bool alive) { alive_ = alive; }

bool HealerActor::IsAlive() const { return alive_; }

void HealerActor::SetTargetPosition(const Vector3& pos) { targetPosition_ = pos; }

const Vector3& HealerActor::GetTargetPosition() const { return targetPosition_; }


void HealerActor::UpdateOBB() {
	obb_.center = transform_.translate;
	obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

const OBB& HealerActor::GetOBB() const {
	return obb_;
}

void HealerActor::HandleCollision() {
	alive_ = false;
}
