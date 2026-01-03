#define NOMINMAX
#include "ResultScene.h"

#include "engine/IrufemiEngine.h"
#include "scene/SceneManager.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "2D/Circle2D.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "manager/DebugUI.h"
#include "function/Function.h"

#include <memory>

ResultScene::~ResultScene() {

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
    
    // --- ライトの初期化 ---
    pointLight_ = std::make_unique <PointLight>();
    pointLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight_->position = { 0.0f, 5.0f, 0.0f };
    pointLight_->intensity = 1.0f;
    pointLight_->radius = 10.0f;
    pointLight_->decay = 1.0f;

    spotLight_ = std::make_unique <SpotLight>();
    spotLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    spotLight_->position = { 2.0f, 1.25f, 0.0f };
    spotLight_->distance = 7.0f;
    spotLight_->direction = Math::Normalize(Vector3{ -1.0f,-1.0f,0.0f });
    spotLight_->intensity = 0.0f; // 初期状態ではOFF
    spotLight_->decay = 2.0f;
    spotLight_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;
}

void ResultScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("ResultScene");
    // pointLight 
    if (ImGui::CollapsingHeader("PointLight")) {
        ImGui::ColorEdit4("PointLightColor", &pointLight_->color.x);
        ImGui::DragFloat3("PointLightPosition", &pointLight_->position.x, 0.01f);
        ImGui::DragFloat("PointLightIntensity", &pointLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat("PointLightRadius", &pointLight_->radius, 0.01f, 0.0f);
        ImGui::DragFloat("PointLightDecay", &pointLight_->decay, 0.01f, 0.0f);
    }
    // spotLight 
    if (ImGui::CollapsingHeader("SpotLight")) {
        ImGui::ColorEdit4("SpotLightColor", &spotLight_->color.x);
        ImGui::DragFloat3("SpotLightPosition", &spotLight_->position.x, 0.01f);
        ImGui::DragFloat("SpotLightIntensity", &spotLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat3("SpotLightDirection", &spotLight_->direction.x, 0.01f);
        spotLight_->direction = Math::Normalize(spotLight_->direction);
        ImGui::DragFloat("SpotLightDistance", &spotLight_->distance, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightDecay", &spotLight_->decay, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightCosAngle", &spotLight_->cosAngle, 0.01f, 0.0f, 1.0f);
    }

    ImGui::End();

#endif // USE_IMGUI

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    if (circle_) { 
        circle_->Debug("ResultCenter");
        circle_->Update();
    }

    //エンターキーが押されていたら
    if (engine_->GetInputManager()->IsKeyPressed(VK_RETURN)) {
        engine_->GetSceneManager()->Request("Title");
    }

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();
    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, *pointLight_, *spotLight_);
}

void ResultScene::Draw() {

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplySpritePSO();

}
