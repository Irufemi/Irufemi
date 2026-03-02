#include "SelectScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/AreaLight.h"
#include "math/DirectionalLight.h"


#include "StageDataManager.h"

SelectScene::~SelectScene() {

}

void SelectScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // カメラ(2D 正射影)
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique <DebugCamera>();
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

    // タイトル文字 の初期化
    text_title_ = std::make_unique<Sprite>();
    text_title_->Initialize(camera_.get(), "resources/texture/stageSelect_title.png");
    text_title_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    // 文字(1)
    text_1_ = std::make_unique<Sprite>();
    text_1_->Initialize(camera_.get(), "resources/texture/stageSelect_1.png");
    text_1_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    // 文字(2)
    text_2_ = std::make_unique<Sprite>();
    text_2_->Initialize(camera_.get(), "resources/texture/stageSelect_2.png");
    text_2_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    // 管理用ベクターに登録
    stageSprites_.push_back(text_1_.get());
    stageSprites_.push_back(text_2_.get());

    // フェードの初期化
    fade_ = std::make_unique<Fade>();
    fade_->Initialize(camera_.get());
    fade_->FadeIn(1.0f, { 0.0f, 0.0f, 0.0f, 1.0f }); // 黒からフェードイン

    // bgm
    bgm_ = std::make_unique<Bgm>();
    bgm_->Initialize("resources/bgm/title.mp3");
    bgm_->PlayFixed();
    // se(決定音)
    se_select_ = std::make_unique<Se>();
    se_select_->Initialize("resources/se/se_select.mp3");
}

void SelectScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("SelectScene");
    if (ImGui::BeginTabBar("SelectSceneTabs")) {

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
    if (debugMode_) {
        // デバッグカメラを更新
        debugCamera_->Update();
        // デバッグカメラの計算結果をメインカメラに上書きする
        const Camera& dbgCam = debugCamera_->GetCamera();
        camera_->SetViewMatrix(dbgCam.GetViewMatrix());
        camera_->SetTranslate(dbgCam.GetTranslate());
        camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
    }
    else {
        // 通常カメラの更新
        camera_->Update("Camera");
    }

    // =====
    // ↓ゲームの更新
    // =====

    // フェードの更新
    fade_->Update();
    if (!fade_->IsDone() && phase_ != Phase::FadingOut) {
        return; // フェードイン中は他の処理をスキップ
    }

    // スプライトの更新
    text_title_->Update();
    for (auto& sprite : stageSprites_) {
        sprite->Update();
    }

    const float BLINK_SPEED = 3.0f;
    const float CONFIRM_BLINK_SPEED = 15.0f;
    const float CONFIRMATION_DURATION = 0.8f;

    if (phase_ == Phase::Selecting) {
        // 入力処理
        if (IScene::PressedVK('A') || engine_->GetInputManager()->GetGamePad()->IsButtonPressed(XINPUT_GAMEPAD_DPAD_LEFT)) {
            currentStageIndex_ = (currentStageIndex_ - 1 + static_cast<int>(stageSprites_.size())) % static_cast<int>(stageSprites_.size());
        }
        if (IScene::PressedVK('D') || engine_->GetInputManager()->GetGamePad()->IsButtonPressed(XINPUT_GAMEPAD_DPAD_RIGHT)) {
            currentStageIndex_ = (currentStageIndex_ + 1) % static_cast<int>(stageSprites_.size());
        }

        // 決定処理
        if (IScene::PressedVK(VK_SPACE) || engine_->GetInputManager()->GetGamePad()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            se_select_->Play();
            phase_ = Phase::Confirming;
            confirmationTimer_ = 0.0f;
            StageDataManager::selectedStageIndex = currentStageIndex_;
        }

        // 明滅処理
        blinkTimer_ += 1.0f / 60.0f;
        float alpha = 0.5f + 0.5f * std::sin(blinkTimer_ * BLINK_SPEED);

        for (size_t i = 0; i < stageSprites_.size(); ++i) {
            if (i == currentStageIndex_) {
                stageSprites_[i]->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
            } else {
                stageSprites_[i]->SetColor({ 0.5f, 0.5f, 0.5f, 1.0f }); // 非選択項目は暗く
            }
        }

    } else if (phase_ == Phase::Confirming) {
        confirmationTimer_ += 1.0f / 60.0f;
        float alpha = std::fmod(confirmationTimer_ * CONFIRM_BLINK_SPEED, 1.0f) > 0.5f ? 1.0f : 0.2f;
        stageSprites_[currentStageIndex_]->SetColor({ 1.0f, 1.0f, 1.0f, alpha });

        if (confirmationTimer_ >= CONFIRMATION_DURATION) {
            phase_ = Phase::FadingOut;
            fade_->FadeOut(1.0f, { 0.0f, 0.0f, 0.0f, 1.0f }); // ゲームシーンへ
        }
    } else if (phase_ == Phase::FadingOut) {
        if (fade_->IsDone()) {
            engine_->GetSceneManager()->Request("InGame");
        }
    }



    // =====
    // ↑ゲームの更新
    // =====

    // エンターキー/Aボタンが押されていたらゲームへ
    if (engine_->GetInputManager()->IsKeyPressed(VK_RETURN) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->Request("InGame");
    }

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = camera_->GetViewMatrix();
    cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = camera_->GetTranslate();

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

void SelectScene::Draw() {

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    text_title_->Draw();
    for (auto& sprite : stageSprites_) {
        sprite->Draw();
    }

    fade_->Draw();
}
