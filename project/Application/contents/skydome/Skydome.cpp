#include "Skydome.h"

#include <cassert>
#include <cmath>

#include "Engine/Graphics/Camera/Camera.h"

#include "Engine/Manager/DebugUI.h"

void Skydome::Initialize() {

	// 引数として受け取ったデータをメンバ変数に記録する
	// ワールド変換の初期化

	model_ = std::make_unique<ObjClass>();
	model_->Initialize("skydome.obj");
	model_->SetEnableLightingToAllMeshes(0);

	worldTransform_ = { Vector3{1.0f,1.0f,1.0f},Vector3{0.0f,0.0f,0.0f},Vector3{0.0f,0.0f,0.0f} };
}

void Skydome::Update() {

	// タイマーを進める（固定フレームレート想定）
	timer_ += 1.0f / 60.0f;
    
	// 1. Y軸の回転を蓄積
	baseRotY_ += rotationSpeedY_;
    
	// 2. X/Z軸にサイン波で微小なゆらぎを計算（周期を少しずらして自然に）
	float wobbleX = std::sin(timer_ * wobbleSpeed_) * wobbleAmplitude_;
	float wobbleZ = std::cos(timer_ * wobbleSpeed_ * 0.8f) * wobbleAmplitude_;
    
	// 3. トランスフォームに適用
	worldTransform_.rotate.x = baseTilt_.x + wobbleX;
	worldTransform_.rotate.z = baseTilt_.z + wobbleZ;
	worldTransform_.rotate.y = baseRotY_;

#ifdef USE_IMGUI

	// DebugUI::DebugTransform(worldTransform_);

#endif // USE_IMGUI

}

void Skydome::Draw() {

	model_->SetTransform(worldTransform_);

	// 3Dモデル描画
	model_->Draw();
}

void Skydome::SetColor(const Vector4& color) {
	if (model_) {
		model_->SetColor(color);
	}
}

void Skydome::AddRotateY(float rotY) {
	worldTransform_.rotate.y += rotY;
}