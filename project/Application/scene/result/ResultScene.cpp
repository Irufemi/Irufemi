#define NOMINMAX
#include "ResultScene.h"

#include "engine/IrufemiEngine.h"
#include "scene/SceneManager.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "2D/Sprite.h" // Spriteをインクルード
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "manager/DebugUI.h"
#include "function/Function.h"
#include "GameResultManager.h" // GameResultManagerをインクルード
#include <memory>
#include <cmath>

ResultScene::~ResultScene() {

}

void ResultScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // カメラ（2D 正射影）
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
    camera_->UpdateMatrix();

    // 結果画像の初期化
    resultImage_ = std::make_unique<Sprite>();
    std::string texturePath;
    if (GameResultManager::result == GameResultManager::Result::Win) {
        texturePath = "resources/texture/Clear.png";
    } else {
        texturePath = "resources/texture/GameOver.png";
    }
    resultImage_->Initialize(camera_.get(), texturePath);
    resultImage_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);

    // Continueテキストの初期化
    continueText_ = std::make_unique<Sprite>();
    continueText_->Initialize(camera_.get(), "resources/texture/titleText_pushKey.png");
    continueText_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() * 0.8f);

    // フェードの初期化
    fade_ = std::make_unique<Fade>();
    fade_->Initialize(camera_.get());
    // リザルトシーン開始時にフェードイン
    if (GameResultManager::result == GameResultManager::Result::Win) {
        fade_->FadeIn(1.0f, { 1.0f, 1.0f, 1.0f, 1.0f }); // 白から
    } else {
        fade_->FadeIn(1.0f, { 0.0f, 0.0f, 0.0f, 1.0f }); // 黒から
    }
    
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
    bgm_->Initialize("resources/bgm/result.mp3");
    bgm_->PlayFixed();
    // se(決定音)
    se_select_ = std::make_unique<Se>();
    se_select_->Initialize("resources/se/se_select.mp3");
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
    Camera* currentCamera = debugMode ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera");

    // フェードの更新
    fade_->Update();
    if (!fade_->IsDone()) {
        resultImage_->Update();
        continueText_->Update();
        return; // フェード中は他の処理をスキップ
    }

    // Continueテキストの明滅
    blinkTimer_ += 1.0f / 60.0f;
    float alpha = 0.5f + 0.5f * std::sin(blinkTimer_ * 5.0f);
    continueText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });

    resultImage_->Update();
    continueText_->Update();

    //エンターキーが押されていたらステージ選択へ
    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->Request("Select");
        se_select_->Play();
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
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    if (resultImage_) {
        resultImage_->Draw();
    }
    if (continueText_ && fade_->IsDone()) {
        continueText_->Draw();
    }

    fade_->Draw();
}
