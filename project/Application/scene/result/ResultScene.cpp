#define NOMINMAX
#include "ResultScene.h"

#include "engine/IrufemiEngine.h"
#include "scene/SceneManager.h"
#include "camera/Camera.h"
#include "2D/Circle2D.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "manager/DebugUI.h"
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

    //Pキーが押されていたら
    if (engine_->GetInputManager()->IsKeyPressed('P')) {
        engine_->GetSceneManager()->Request("Title");
    }
}

void ResultScene::Draw() {
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplySpritePSO();
}
