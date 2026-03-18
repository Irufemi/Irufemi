#include "Skydome.h"

#include <cassert>

#include "camera/Camera.h"

#include "Engine/Manager/DebugUI.h"

void Skydome::Initialize(Camera* camera) {

	// 引数として受け取ったデータをメンバ変数に記録する
	// ワールド変換の初期化
	this->camera_ = camera;

	model_ = std::make_unique<ObjClass>();
	model_->Initialize(camera_, "skydome.obj");
	model_->SetEnableLightingToAllMeshes(0);

	worldTransform_ = { Vector3{1.0f,1.0f,1.0f},Vector3{0.0f,0.0f,0.0f},Vector3{0.0f,0.0f,0.0f} };
}

void Skydome::Update() {

#ifdef USE_IMGUI

	// DebugUI::DebugTransform(worldTransform_);

#endif // USE_IMGUI

}

void Skydome::Draw() {

	model_->SetTransform(worldTransform_);

	model_->Update();

	// 3Dモデル描画
	model_->Draw();
}