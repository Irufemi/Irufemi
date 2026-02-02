#include "Wall.h"

#include "camera/Camera.h"
#include "3D/ObjClass.h"
#include "function/Math.h"

Wall::Wall() {}

Wall::~Wall() {}

void Wall::Initialize(Camera* camera, const Vector3& pos) {
	
	camera_ = camera;

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera,"TD_Block.obj");

	transform_.translate = pos;
	ringIndex_ = 0; // デフォルトは0層
}

void Wall::Update() {
	UpdateOBB();
}

void Wall::Draw() { 
	// 描画物の更新
	model_->SetTransform(transform_);
	model_->Update();
	// 描画
	if (model_) model_->Draw();
}

const Vector3& Wall::GetPosition() const { return transform_.translate; }

void Wall::SetRotation(const Vector3& rot) { transform_.rotate = rot; }

void Wall::SetScale(const Vector3& scale) { transform_.scale = scale; }

const Vector3& Wall::GetRotation() const { return transform_.rotate; }

bool Wall::AccumulateContactFrame() {
	++contactFrames_;
	if (contactFrames_ >= kRequiredContactFrames_) {
		contactFrames_ = 0;
		--hp_;
		if (hp_ <= 0) {
			return true;
		}
	}
	return false;
}

void Wall::DecayContactFrames() {
	if (contactFrames_ > 0) {
		contactFrames_--;
	}
}

void Wall::UpdateOBB() {
	obb_.center = transform_.translate;
	obb_.size = { (width_ * transform_.scale.x) / 2.0f, (height_ * transform_.scale.y) / 2.0f, (depth_ * transform_.scale.z) / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };

	// 各軸を正規化
	obb_.orientations[0] = Math::Normalize(obb_.orientations[0]);
	obb_.orientations[1] = Math::Normalize(obb_.orientations[1]);
	obb_.orientations[2] = Math::Normalize(obb_.orientations[2]);
}

const OBB& Wall::GetOBB() const {
	return obb_;
}

const Transform& Wall::GetTransform() const {
	return transform_;
}

Vector3 Wall::GetSize() const {
	return { width_ * transform_.scale.x, height_ * transform_.scale.y, depth_ * transform_.scale.z };
}
