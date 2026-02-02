#include "Wall.h"

#include "camera/Camera.h"
#include "3D/ObjClass.h"
#include "function/Math.h"
#include "audio/Se.h"

Wall::Wall() {}

Wall::~Wall() {}

void Wall::Initialize(Camera* camera, const Vector3& pos) {
	
	camera_ = camera;

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera,"TD_Block.obj");

	transform_.translate = pos;
	ringIndex_ = 0; // デフォルトは0層
	playedRepairSE_ = false; // reset per-instance flag
}

void Wall::Update() {

	// --- ここから修復演出の更新 ---
	if (isRepairing_) {
		// 1フレーム分進める（60FPS前提なら 1/60）
		repairAnimTimer_ += 1.0f / 60.0f;
		float t = repairAnimTimer_ / repairAnimDuration_;

		if (t >= 1.0f) {
			// 演出完了
			t = 1.0f;
			isRepairing_ = false;
			repairAlpha_ = 1.0f;
			transform_.scale = repairBaseScale_;
		} else {
			// α値：線形で0→1
			repairAlpha_ = t;

			// スケール：バウンスイージングで0→1
			float bounceT = BounceEaseOut(t);
			transform_.scale = {
				repairBaseScale_.x * bounceT,
				repairBaseScale_.y * bounceT,
				repairBaseScale_.z * bounceT
			};
		}
	}

	// --- ここまで修復演出 ---
	UpdateOBB();
}

void Wall::Draw() { 
	// 描画物の更新
	model_->SetTransform(transform_);
	model_->SetAlpha(repairAlpha_);
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

void Wall::StartRepairAnimation(){
	isRepairing_ = true;
	repairAnimTimer_ = 0.0f;
	repairAlpha_ = 0.0f;
	// 目標スケールを保存しておき、アニメ中は0から始める
	repairBaseScale_ = transform_.scale;
	transform_.scale = { 0.0f, 0.0f, 0.0f };

	
	static Se seRepair;
	static bool seInitialized = false;
	if (!seInitialized) {
		
		seRepair.Initialize("resources/audio/se/Repair.mp3", "", 0.2f);
		seInitialized = true;
	}

	
	if (!playedRepairSE_) {
		seRepair.SetVolume(0.2f);
		seRepair.Play(false);
		playedRepairSE_ = true;
	}
}

float Wall::BounceEaseOut(float t)
{
	// バウンスイージング
	if (t < 1.0f / 2.75f) {
		return 7.5625f * t * t;
	} else if (t < 2.0f / 2.75f) {
		t -= 1.5f / 2.75f;
		return 7.5625f * t * t + 0.75f;
	} else if (t < 2.5f / 2.75f) {
		t -= 2.25f / 2.75f;
		return 7.5625f * t * t + 0.9375f;
	} else {
		t -= 2.625f / 2.75f;
		return 7.5625f * t * t + 0.984375f;
	}
}
