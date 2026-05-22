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
	// CyberHexParamsの初期設定
	cyberHexParams_.edgeColor = { 0.0f, 0.8f, 1.0f, 1.0f }; // サイバーブルー
	cyberHexParams_.edgeThickness = 0.05f; // 縁の太さを元の 1/2 にする
	cyberHexParams_.baseBrightness = 0.15f;
	cyberHexParams_.flickerAmplitude = 0.4f;
	cyberHexParams_.distortion = 0.05f;
	cyberHexParams_.density = 0.01f;        // 元 uvTransform.m[0][0]
	cyberHexParams_.animationSpeed = 0.2f;  // 元 uvTransform.m[1][1]
	cyberHexParams_.uvScrollX = 0.0f;
	cyberHexParams_.uvScrollY = 0.0f;

	cyberHexCB_ = std::make_unique<DynamicConstantBuffer<CyberHexParams>>();
	cyberHexCB_->Initialize(engine_->GetDirectXCommon(), 1);
	cyberHexCBIndex_ = cyberHexCB_->Allocate();
	for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
		cyberHexCB_->Update(cyberHexCBIndex_, cyberHexParams_, i);
	}
	pFloor_ = std::make_unique<PrimitiveObjects3DClass>();
	pFloor_->Initialize(PrimitiveType::Plane);
	pFloor_->SetScale({ 200.0f,200.0f,200.0f });
	pFloor_->SetRotate({ 1.57f,0.0f, 0.0f });
	pFloor_->SetPosition({ 0.0f,0.0f,0.0f });
	pFloor_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f }); // 基本カラー
	pFloor_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));
	pPZWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pPZWall_->Initialize(PrimitiveType::Plane);
	pPZWall_->SetScale({ 200.0f,20.0f,200.0f });
	pPZWall_->SetPosition({ 0.0f,10.0f,100.0f });
	pPZWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pPZWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pMZWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pMZWall_->Initialize(PrimitiveType::Plane);
	pMZWall_->SetScale({ 200.0f,20.0f,200.0f });
	pMZWall_->SetRotate({ 0.0f,3.14f, 0.0f });
	pMZWall_->SetPosition({ 0.0f,10.0f,-100.0f });
	pMZWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pMZWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pPXWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pPXWall_->Initialize(PrimitiveType::Plane);
	pPXWall_->SetScale({ 200.0f,20.0f,200.0f });
	pPXWall_->SetRotate({ 0.0f,1.57f, 0.0f });
	pPXWall_->SetPosition({ 100.0f,10.0f,0.0f });
	pPXWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pPXWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	pMXWall_ = std::make_unique<PrimitiveObjects3DClass>();
	pMXWall_->Initialize(PrimitiveType::Plane);
	pMXWall_->SetScale({ 200.0f,20.0f,200.0f });
	pMXWall_->SetRotate({ 0.0f,-1.57f, 0.0f });
	pMXWall_->SetPosition({ -100.0f,10.0f,0.0f });
	pMXWall_->SetColor({ 0.0f, 0.8f, 1.0f, 1.0f });
	pMXWall_->SetCustomPSO(engine_->GetPSOManager()->GetPSO("CyberHex", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));

	building_ = std::make_unique<Building>();
	building_->Initialize(engine_);
}

void Field::Update() {
	if (cyberHexCB_) {
		uint32_t frameIndex = engine_->GetDirectXCommon()->GetFrameIndex();
		cyberHexCB_->Update(cyberHexCBIndex_, cyberHexParams_, frameIndex);
		D3D12_GPU_VIRTUAL_ADDRESS addr = cyberHexCB_->GetGPUVirtualAddress(cyberHexCBIndex_, frameIndex);
		if (pFloor_) pFloor_->SetCustomCBVAddress(addr);
		if (pPZWall_) pPZWall_->SetCustomCBVAddress(addr);
		if (pMZWall_) pMZWall_->SetCustomCBVAddress(addr);
		if (pPXWall_) pPXWall_->SetCustomCBVAddress(addr);
		if (pMXWall_) pMXWall_->SetCustomCBVAddress(addr);
	}
	if (pFloor_) pFloor_->Update();
	if (pPZWall_) pPZWall_->Update();
	if (pMZWall_) pMZWall_->Update();
	if (pPXWall_) pPXWall_->Update();
	if (pMXWall_) pMXWall_->Update();

	if (building_) {
		building_->Update();
	}

#if defined USE_IMGUI
	if (ImGui::Begin("CyberHex Settings")) {
		ImGui::ColorEdit3("Edge Color", &cyberHexParams_.edgeColor.x);
		ImGui::SliderFloat("Edge Thickness", &cyberHexParams_.edgeThickness, 0.01f, 0.5f);
		ImGui::SliderFloat("Base Brightness", &cyberHexParams_.baseBrightness, 0.0f, 1.0f);
		ImGui::SliderFloat("Flicker Amplitude", &cyberHexParams_.flickerAmplitude, 0.0f, 1.0f);
		ImGui::SliderFloat("Distortion", &cyberHexParams_.distortion, 0.0f, 0.2f);
		ImGui::SliderFloat("Density", &cyberHexParams_.density, 0.001f, 0.1f, "%.4f");
		ImGui::SliderFloat("Animation Speed", &cyberHexParams_.animationSpeed, 0.0f, 2.0f);
		ImGui::SliderFloat("UV Scroll X", &cyberHexParams_.uvScrollX, -0.5f, 0.5f);
		ImGui::SliderFloat("UV Scroll Y", &cyberHexParams_.uvScrollY, -0.5f, 0.5f);
	}
	ImGui::End();



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

	if (building_ && drawBuildings_) {
		building_->Draw(engine_);
	}
}