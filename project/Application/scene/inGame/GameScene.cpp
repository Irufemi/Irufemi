#include "GameScene.h"

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
GameScene::~GameScene() {

}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {
    // エンジンへのポインタを保持
    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f,0.0f,-10.0f });

    // 重要：SetTranslate の後で行列を確実に更新しておく
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;

    // ポーズ画面
    pauseSprite_ = std::make_unique<Sprite>();
    pauseSprite_->Initialize(camera_.get(), "resources/whiteTexture.png");
    pauseSprite_->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);
    pauseSprite_->SetSize(static_cast<float>(engine_->GetClientWidth()), static_cast<float>(engine_->GetClientHeight()));
    pauseSprite_->SetAnchor(0.5f, 0.5f);
    pauseSprite_->SetColor({ 0.1f, 0.1f, 0.1f, 0.5f });
}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("GameScene");
    if (ImGui::BeginTabBar("GameSceneTabs")) {

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

#endif // _DEBUG

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    

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

// 描画
void GameScene::Draw() {

}

void GameScene::PauseUpdate()
{

    // ポーズ中の更新処理
    if (pauseSprite_) {
        pauseSprite_->Update();
    }

}

void GameScene::PauseDraw()
{
    // ポーズ画面の描画
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // 半透明の背景
    if (pauseSprite_) {
        pauseSprite_->Draw();
    }
}