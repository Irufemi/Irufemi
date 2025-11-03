#define NOMINMAX
#include "ResultScene.h"

#include "engine/IrufemiEngine.h"
#include "camera/Camera.h"
#include "2D/Circle2D.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "externals/imgui/imgui.h"
#include "function/Function.h"

#include <memory>

ResultScene::~ResultScene() {
	// シーン破棄時に DrawManager の参照を外す
	if (engine_) {
		if (engine_->GetDrawManager()) {
			engine_->GetDrawManager()->SetPointLightClass(nullptr);
			engine_->GetDrawManager()->SetSpotLightClass(nullptr);
		}
	}
}

void ResultScene::Initialize(IrufemiEngine* engine) {
	engine_ = engine;

	// カメラ（2D 正射影）
	camera_ = std::make_unique<Camera>();
	camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
	camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
	camera_->UpdateMatrix();

	// Circle2D の初期化
	circle_ = std::make_unique<Circle2D>();
	circle_->Initialize(camera_.get(), "");
	float cx = static_cast<float>(engine_->GetClientWidth()) * 0.5f;
	float cy = static_cast<float>(engine_->GetClientHeight()) * 0.5f;
	circle_->SetInfo({ Vector3{ cx, cy, 0.0f }, 50.0f });
	circle_->SetUseTexture(false);
	circle_->SetColor(Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });

	// ワークアラウンド：DrawManager に渡すライトを用意しておく
	pointLight_ = std::make_unique<PointLightClass>();
	pointLight_->Initialize();
	pointLight_->SetPos(Vector3{ 0.0f, 30.0f, 0.0f });
	engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

	spotLight_ = std::make_unique<SpotLightClass>();
	spotLight_->Initialize();
	spotLight_->SetIntensity(0.0f);
	engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());
}

void ResultScene::Update() {
	if (circle_) circle_->Update("ResultCenter");

	//エンターキーが押されていたら
	if (engine_->GetInputManager()->IsKeyPressed(VK_RETURN)) {
		if (g_SceneManager) {
			g_SceneManager->Request(SceneName::title);
		}
	}
}

void ResultScene::Draw() {
	engine_->SetBlend(BlendMode::kBlendModeNormal);
	engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
	engine_->ApplySpritePSO();
	if (circle_) circle_->Draw();
}
