#include "ResultScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "2D/Sprite.h"

#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "math/AreaLight.h"

#include "scene/GameResultData.h"
#include "engine/Input/InputManager.h"

ResultScene::~ResultScene() {
    // ResultScene 終了時に BGM を停止
    bgmResult_.Stop();
}

void ResultScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // カメラ(2D 正射影)
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
    camera_->UpdateMatrix();

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

    // --- スプライトの初期化 ---
    // テクスチャパスは仮のものです
    gameClearModel_ = std::make_unique<ObjClass>();
    gameClearModel_->Initialize(camera_.get(), "TD_Clear.obj");

    gameOverModel_ = std::make_unique<ObjClass>();
    gameOverModel_->Initialize(camera_.get(), "TD_GameOver.obj");
    gameOverModel_->Update();

    // --- 結果データに基づいて表示を決定 ---
    auto& resultData = GameResultData::GetInstance();
    if (resultData.isGameClear) {
        currentResultModel_ = gameClearModel_.get();
    }
    else {
        currentResultModel_ = gameOverModel_.get();
    }

    // --- Result BGM の初期化と再生 ---
    // ゲームオーバー/クリア画面に応じて適切なBGMを再生
    if (resultData.isGameClear) {
        bgmResult_.Initialize("resources/audio/bgm/GameClear.mp3", "", true, true);
    } else {
        bgmResult_.Initialize("resources/audio/bgm/GameOver.mp3", "", true, true);
    }
    bgmResult_.SetVolume(1.0f);

    // --- トランジションの初期化 ---
    stripeTransition_ = std::make_unique<StripeTransition>();
    stripeTransition_->Initialize(camera_.get(), engine_, StripeTransition::Mode::Out);
    stripeTransition_->Start();

    isTransitioningToTitle_ = false;
}

void ResultScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("ResultScene");

    if (ImGui::BeginTabBar("ResultSceneTabs")) {

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

#endif // USE_IMGUI

    // --- カメラの更新 ---
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera");

    // =====
    // ↓ゲームの更新
    // =====

    stripeTransition_->Update();

    if (currentResultModel_) {
        currentResultModel_->Update();
    }

    // 何かキーが押されたらタイトルに戻る
    if (!isTransitioningToTitle_ && stripeTransition_->IsFinished()) {
        if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            isTransitioningToTitle_ = true;
            stripeTransition_->Initialize(camera_.get(), engine_, StripeTransition::Mode::In);
            stripeTransition_->Start();
            // トランジション開始時にBGMをフェードアウト的に停止
            bgmResult_.Stop();
        }
    }

    if (isTransitioningToTitle_ && stripeTransition_->IsFinished()) {
        engine_->GetSceneManager()->Request("Title");
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

void ResultScene::Draw() {

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyPSO();

    if (currentResultModel_) {
        currentResultModel_->Draw();
    }

    engine_->ApplySpritePSO();

    stripeTransition_->Draw();
}
