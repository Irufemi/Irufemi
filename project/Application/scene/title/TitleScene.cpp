#include "TitleScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"

#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"

// デストラクタ
TitleScene::~TitleScene() {

}

// 初期化
void TitleScene::Initialize(IrufemiEngine* engine) {

    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f,0.0f,-10.0f });

    // 重要：SetTranslate の後で行列を確実に更新しておく
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

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

// 更新
void TitleScene::Update() {


#if defined USE_IMGUI

    ImGui::Begin("TitleScene");
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
    // directionalLight
    if (ImGui::CollapsingHeader("DirectionalLight")) {
        ImGui::ColorEdit4("DirectionalLightColor", &directionalLight_->color.x);
        ImGui::DragFloat3("DirectionalLightDirection", &directionalLight_->direction.x, 0.01f);
        directionalLight_->direction = Math::Normalize(directionalLight_->direction);
        ImGui::DragFloat("DirectionalLightIntensity", &directionalLight_->intensity, 0.01f, 0.0f);
    }
    ImGui::Checkbox("debugMode", &debugMode_);

    ImGui::End();

#endif // USE_IMGUI

    // --- カメラの更新 ---
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera");

    // =====
    // ↓ゲームの更新
    // =====

    // =====
    // ↑ゲームの更新
    // =====

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();
    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, *pointLight_, *spotLight_);
}

void TitleScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();


}