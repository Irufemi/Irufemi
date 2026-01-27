#include "Sword.h"
#include "3D/ObjClass.h"
#include <memory>

void Sword::Initialize(Camera* camera, const Vector3& pos) {
	camera_ = camera;

	transform_.translate = pos;
	
	CreateObj(camera);
	if (swordModel_) {
		swordModel_->SetPosition(pos);
	}
}

void Sword::Update() {

	if (swordModel_) {
		swordModel_->Update();
	}
}

void Sword::Draw() {
	if (swordModel_) {
		swordModel_->Update();
		swordModel_->Draw();
	}
}

void Sword::CreateObj(Camera* camera) {
	
	swordModel_ = std::make_unique<ObjClass>();
	swordModel_->Initialize(camera, "TD_Sword.obj");
}

void Sword::UpdateOBB() {
	obb_.center = transform_.translate;
	obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

void Sword::SetPosition(const Vector3& pos) {
    if (swordModel_) swordModel_->SetPosition(pos);
}

void Sword::SetTransform(const Transform& t) {
    if (swordModel_) swordModel_->SetTransform(t);
}
