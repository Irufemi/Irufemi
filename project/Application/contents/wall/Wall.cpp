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

void Wall::UpdateOBB() {
	obb_.center = transform_.translate;
	obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

const OBB& Wall::GetOBB() const {
	return obb_;
}

const Vector3& Wall::GetPosition() const { return transform_.translate; }

void Wall::SetRotation(const Vector3& rot) { transform_.rotate = rot; }

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
	// 被触続していないときに徐々に接触フレームを減少させる
	if (contactFrames_ > 0) {
		contactFrames_ -= 1; // 1フレーム分だけ減らす。必要なら緩やかに変更。
		if (contactFrames_ < 0) contactFrames_ = 0;
	}
}
