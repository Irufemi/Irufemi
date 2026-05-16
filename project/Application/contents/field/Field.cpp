#include "Field.h"

#include "Engine/Graphics/Camera/Camera.h"
#include "building/building.h"

Field::Field(IrufemiEngine* engine) {
	engine_ = engine;
	input_ = engine_->GetInputManager();
}

Field::~Field() {
}

void Field::Initialize() {
	pFloor_ = std::make_unique<PrimitiveObjects3DClass>();
	pFloor_->Initialize(PrimitiveType::Plane);
	pFloor_->SetScale({ 200.0f,200.0f,200.0f });
	pFloor_->SetRotate({ 1.57f,0.0f, 0.0f });
	pFloor_->SetPosition({ 0.0f,0.0f,0.0f });
	pFloor_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f }); // サイバーブルー
	pFloor_->GetMaterial().uvTransform.m[0][0] = 0.01f; // ヘキサゴンの密度（大きいほど細かい）
	pFloor_->GetMaterial().uvTransform.m[1][1] = 0.2f;  // アニメーション速度（小さいほどゆっくり）
	pFloor_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pPZWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pPZWall_->Initialize(PrimitiveType::Plane);
	pPZWall_->SetScale({ 200.0f,20.0f,200.0f });
	pPZWall_->SetPosition({ 0.0f,10.0f,100.0f });
	pPZWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pPZWall_->GetMaterial().uvTransform.m[0][0] = 0.01f;
	pPZWall_->GetMaterial().uvTransform.m[1][1] = 0.2f;
	pPZWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pMZWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pMZWall_->Initialize(PrimitiveType::Plane);
	pMZWall_->SetScale({ 200.0f,20.0f,200.0f });
	pMZWall_->SetRotate({ 0.0f,3.14f, 0.0f });
	pMZWall_->SetPosition({ 0.0f,10.0f,-100.0f });
	pMZWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pMZWall_->GetMaterial().uvTransform.m[0][0] = 0.01f;
	pMZWall_->GetMaterial().uvTransform.m[1][1] = 0.2f;
	pMZWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pPXWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pPXWall_->Initialize(PrimitiveType::Plane);
	pPXWall_->SetScale({ 200.0f,20.0f,200.0f });
	pPXWall_->SetRotate({ 0.0f,1.57f, 0.0f });
	pPXWall_->SetPosition({ 100.0f,10.0f,0.0f });
	pPXWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pPXWall_->GetMaterial().uvTransform.m[0][0] = 0.01f;
	pPXWall_->GetMaterial().uvTransform.m[1][1] = 0.2f;
	pPXWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pMXWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pMXWall_->Initialize(PrimitiveType::Plane);
	pMXWall_->SetScale({ 200.0f,20.0f,200.0f });
	pMXWall_->SetRotate({ 0.0f,-1.57f, 0.0f });
	pMXWall_->SetPosition({ -100.0f,10.0f,0.0f });
	pMXWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pMXWall_->GetMaterial().uvTransform.m[0][0] = 0.01f;
	pMXWall_->GetMaterial().uvTransform.m[1][1] = 0.2f;
	pMXWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	building_ = std::make_unique<Building>();
	building_->Initialize(engine_);
}

void Field::Update() {
	if (pFloor_) pFloor_->Update();
	if (pPZWall_) pPZWall_->Update();
	if (pMZWall_) pMZWall_->Update();
	if (pPXWall_) pPXWall_->Update();
	if (pMXWall_) pMXWall_->Update();

	if (building_) {
		building_->Update();
	}

#if defined USE_IMGUI
	pFloor_->Debug();
	if (building_) {
		building_->DrawImGui();
	}
#endif
}

void Field::Draw() {
	pFloor_->Draw();
	pPZWall_->Draw();
	pMZWall_->Draw();
	pPXWall_->Draw();
	pMXWall_->Draw();

	if (building_) {
		building_->Draw(engine_);
	}
}