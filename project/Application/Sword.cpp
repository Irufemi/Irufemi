#include "Sword.h"
#include "3D/ObjClass.h"
#include <memory>
#include "function/Math.h"
#include <algorithm>
#include <cmath>

void Sword::Initialize(Camera* camera, const Vector3& pos) {
	camera_ = camera;

	transform_.translate = pos;
	
	CreateObj(camera);
	if (swordModel_) {
		swordModel_->SetPosition(pos);
	}
}

void Sword::Update() {

	
	constexpr float kFrameDt = 1.0f / 60.0f;

	if (isSlashing_) {
		slashTimer_ += kFrameDt;
		float t = std::clamp(slashTimer_ / slashDuration_, 0.0f, 1.0f);
		
		float tt = t * t * (3.0f - 2.0f * t);

		
		const float pi = 3.141592654f;
		float outwardPulse = std::sin(t * pi) * 0.5f; 

		Transform cur = {};

	
		Vector3 posInterp;
		posInterp.x = slashStartTransform_.translate.x + (slashEndTransform_.translate.x - slashStartTransform_.translate.x) * tt;
		posInterp.y = slashStartTransform_.translate.y + (slashEndTransform_.translate.y - slashStartTransform_.translate.y) * tt;
		posInterp.z = slashStartTransform_.translate.z + (slashEndTransform_.translate.z - slashStartTransform_.translate.z) * tt;

		
		float curAngle = slashStartTransform_.rotate.z + (slashEndTransform_.rotate.z - slashStartTransform_.rotate.z) * tt;
		Vector3 forward{ std::sin(-curAngle), std::cos(-curAngle), 0.0f };
		posInterp = posInterp + Math::Multiply(outwardPulse, forward);

		cur.translate = posInterp;

		
		cur.rotate.x = slashStartTransform_.rotate.x + (slashEndTransform_.rotate.x - slashStartTransform_.rotate.x) * tt + 0.4f * std::sin(t * pi);
		cur.rotate.y = slashStartTransform_.rotate.y + (slashEndTransform_.rotate.y - slashStartTransform_.rotate.y) * tt;
		cur.rotate.z = slashStartTransform_.rotate.z + (slashEndTransform_.rotate.z - slashStartTransform_.rotate.z) * tt;

		
		float baseScale = slashStartTransform_.scale.x;
		float scalePulse = 1.0f + 0.7f * std::sin(t * pi) * (1.0f - 0.5f * t); 
		cur.scale = { baseScale * scalePulse, baseScale * scalePulse, baseScale * scalePulse };

		SetTransform(cur);

		if (t >= 1.0f) {
			isSlashing_ = false;
		}
	}

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

	float sx = transform_.scale.x;
	float sy = transform_.scale.y;
	float sz = transform_.scale.z;
	obb_.size = { (width_ * sx) / 2.0f, (height_ * sy) / 2.0f, (depth_ * sz) / 2.0f };
	Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
	obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
	obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
	obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

void Sword::SetPosition(const Vector3& pos) {
    if (swordModel_) swordModel_->SetPosition(pos);
}

void Sword::SetTransform(const Transform& t) {
    transform_ = t;
    if (swordModel_) swordModel_->SetTransform(t);
}

void Sword::StartSlash(const Transform& anchor, float duration) {
	
	isSlashing_ = true;
	slashTimer_ = 0.0f;
	slashDuration_ = duration; 
	float baseAngle = anchor.rotate.z;
	float startAngle = baseAngle - 1.2f;
	float endAngle = baseAngle + 1.2f;

	// increment slash id
	++currentSlashId_;

	//	
	float tipLocal = 1.6f;

	//	slashStartTransform_ = anchor;
	slashStartTransform_ = anchor;
	slashStartTransform_.rotate.z = startAngle;
	{
		Vector3 dirStart{ std::sin(-startAngle), std::cos(-startAngle), 0.0f };
		Vector3 offsetStart = Math::Multiply(tipLocal, dirStart);
		slashStartTransform_.translate = anchor.translate + offsetStart; 
	}

	//	slashEndTransform_ = anchor;
	slashEndTransform_ = anchor;
	slashEndTransform_.rotate.z = endAngle;
	{
		Vector3 dirEnd{ std::sin(-endAngle), std::cos(-endAngle), 0.0f };
		Vector3 offsetEnd = Math::Multiply(tipLocal, dirEnd);
		slashEndTransform_.translate = anchor.translate + offsetEnd;
	}

	
	slashStartTransform_.scale = anchor.scale;
	slashEndTransform_.scale = anchor.scale;

	// コールバックを呼ぶ
	if (onSlashStart_) {
		onSlashStart_(slashStartTransform_);
	}
}
