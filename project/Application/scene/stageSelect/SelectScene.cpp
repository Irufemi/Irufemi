#define NOMINMAX
#include "SelectScene.h"

#include "engine/IrufemiEngine.h"
#include "scene/SceneManager.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "2D/Sprite.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "manager/DebugUI.h"
#include "function/Function.h"
#include "function/Ease.h"
#include "StageDataManager.h"

#include <memory>
#include <cmath> // std::sin, std::fmod

SelectScene::~SelectScene() {

}

void SelectScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // カメラ（2D 正射影）
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
    camera_->UpdateMatrix();

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

    // フェードの更新
    fade_->Update();
    if (!fade_->IsDone() && phase_ != Phase::FadingOut) {
        return; // フェードイン中は他の処理をスキップ
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

    // --- カメラの更新 ---
    Camera* currentCamera = debugMode ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera");

    // スプライトの更新
    text_title_->Update();
    for (auto& sprite : stageSprites_) {
        sprite->Update();
    }

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();
    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, *pointLight_, *spotLight_);
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
