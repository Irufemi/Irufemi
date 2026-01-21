#include "CGScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "math/AreaLight.h"
#include "2D/Sprite.h"

// デストラクタ
CGScene::~CGScene() {

}

// 初期化
void CGScene::Initialize(IrufemiEngine* engine) {

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
    auto pointLight = std::make_unique<PointLight>();
    pointLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight->position = { 0.0f, 5.0f, 0.0f };
    pointLight->intensity = 1.0f;
    pointLight->radius = 10.0f;
    pointLight->decay = 1.0f;
    pointLight->isActive = 1;
    pointLights_.push_back(std::move(pointLight));

    auto spotLight = std::make_unique<SpotLight>();
    spotLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    spotLight->position = { 2.0f, 1.25f, 0.0f };
    spotLight->distance = 7.0f;
    spotLight->direction = Math::Normalize(Vector3{ -1.0f,-1.0f,0.0f });
    spotLight->intensity = 0.0f; // 初期状態ではOFF
    spotLight->decay = 2.0f;
    spotLight->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
    spotLight->isActive = 1;
    spotLights_.push_back(std::move(spotLight));

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;

    auto areaLight = std::make_unique<AreaLight>();
    areaLight->color = { 1.0f, 0.5f, 0.5f, 1.0f };
    areaLight->position = { 0.0f, 2.0f, 2.0f };
    areaLight->intensity = 1.0f;
    areaLight->direction = { 0.0f, -1.0f, 0.0f };
    areaLight->range = 10.0f;
    areaLight->size = { 2.0f, 2.0f };
    areaLight->isActive = 1;
    areaLights_.push_back(std::move(areaLight));

    isActiveAnimationModel_animatedCube_ = false;
    isActiveAnimationModel_walk_ = false;
    isActiveAnimationModel_sneakWalk_ = false;

    if (isActiveAnimationModel_animatedCube_) {
        animationModel_animatedCube_ = std::make_unique<AnimationModel>();
        animationModel_animatedCube_->Initialize(camera_.get(),"sample/AnimatedCube.gltf");
    }
    if (isActiveAnimationModel_walk_) {
        animationModel_walk_ = std::make_unique<AnimationModel>();
        animationModel_walk_->Initialize(camera_.get(), "sample/walk.gltf");
    }
    if (isActiveAnimationModel_sneakWalk_) {
        animationModel_sneakWalk_ = std::make_unique<AnimationModel>();
        animationModel_sneakWalk_->Initialize(camera_.get(), "sample/sneakWalk.gltf");
    }

}

// 更新
void CGScene::Update() {


#ifdef USE_IMGUI

    ImGui::Begin("CGScene");

    if (ImGui::BeginTabBar("CGSceneTabs")) {
        
        DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);

        // Texture タブ
        if (ImGui::BeginTabItem("Texture")) {
            if (ImGui::Button("allLoadActivate")) {
                engine_->GetTextureManager()->LoadAllFromFolder("resources/");
            }
            ImGui::EndTabItem();
        }

        // Debug タブ
        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Checkbox("debugMode", &debugMode_);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Begin("Activation");
    ImGui::Checkbox("AnimationModel_animatedCube", &isActiveAnimationModel_animatedCube_);
    ImGui::Checkbox("AnimationModel_walk", &isActiveAnimationModel_walk_);
    ImGui::Checkbox("AnimationModel_sneakWalk", &isActiveAnimationModel_sneakWalk_);
    ImGui::End();

#endif // USE_IMGUI

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // =====
    // ↓ゲームの更新
    // =====

    // 3D

    if (isActiveAnimationModel_animatedCube_) {
        if (!animationModel_animatedCube_) {
            animationModel_animatedCube_ = std::make_unique<AnimationModel>();
            animationModel_animatedCube_->Initialize(camera_.get(),"sample/AnimatedCube.gltf");
        }
        animationModel_animatedCube_->Debug("animationModel_animatedCube");
        animationModel_animatedCube_->Update();
    }
    if (isActiveAnimationModel_walk_) {
        if (!animationModel_walk_) {
            animationModel_walk_ = std::make_unique<AnimationModel>();
            animationModel_walk_->Initialize(camera_.get(), "sample/walk.gltf");
        }
        animationModel_walk_->Debug("aniamtionModel_walk_");
        animationModel_walk_->Update();
    }
    if (isActiveAnimationModel_sneakWalk_) {
        if (!animationModel_sneakWalk_) {
            animationModel_sneakWalk_ = std::make_unique<AnimationModel>();
            animationModel_sneakWalk_->Initialize(camera_.get(), "sample/sneakWalk.gltf");
        }
        animationModel_sneakWalk_->Debug("aniamtionModel_sneakWalk_");
        animationModel_sneakWalk_->Update();
    }

    // =====
    // ↑ゲームの更新
    // =====

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();

    std::vector<PointLight*> pLights;
    for (const auto& light : pointLights_) {
        pLights.push_back(light.get());
    }
    std::vector<SpotLight*> sLights;
    for (const auto& light : spotLights_) {
        sLights.push_back(light.get());
    }
    std::vector<AreaLight*> aLights;
    for (const auto& light : areaLights_) {
        aLights.push_back(light.get());
    }

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);
}

void CGScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    engine_->ApplyPSO();

    if (isActiveAnimationModel_animatedCube_) {
        animationModel_animatedCube_->Draw();
    }
    if (isActiveAnimationModel_walk_) {
        animationModel_walk_->Draw();
    }
    if (isActiveAnimationModel_sneakWalk_) {
        animationModel_sneakWalk_->Draw();
    }

}