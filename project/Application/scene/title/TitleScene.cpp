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
#include "math/AreaLight.h"

#include "3D/ObjClass.h"
#include "3D/particle/ParticleSystem.h"
#include <random> // std::mt19937 と分布クラスのために追加
// デストラクタ
TitleScene::~TitleScene() = default;

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
    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;

    /// Sprite
    // 押したらスタート
    textSprite_pushStart_ = std::make_unique<Sprite>();
    textSprite_pushStart_->Initialize(camera_.get(), "resources/texture/title/text_pushStart.png");
    textSprite_pushStart_->SetPositionCenter(engine_->GetClientWidth() / 2.0f, 2.0f * engine_->GetClientHeight() / 3.0f);
    textSprite_pushStart_->Update();

#pragma region takamura追加
    // ストライプトランジション初期化（入り）
    stripeTransition_ = std::make_unique<StripeTransition>();
    stripeTransition_->Initialize(camera_.get(), engine_, StripeTransition::Mode::In);
#pragma endregion takamura追加
  
    titleObj_ = std::make_unique<ObjClass>();
  
    titleObj_->Initialize(camera_.get(), "TD_Title.obj");
  
    titleObj_->SetPosition({ 0.0f, 1.0f, 0.0f });
    titleObj_->SetScale({ 1.0f, 1.0f, 1.0f });

    seDecision_.Initialize("resources/audio/se/Decision.mp3");
    bgmTitle_.Initialize("resources/audio/bgm/Title.mp3", "", true, true);
    bgmTitle_.SetVolume(1.0f);

    // 背景パーティクルの初期化
    // 画面のワールド座標範囲を計算 (Z=0平面)
    float fovY = camera_->GetFovAngleY();
    float aspectRatio = camera_->GetAspectRatio();
    float viewHeight = 2.0f * 10.0f * std::tan(fovY * 0.5f);
    float viewWidth = viewHeight * aspectRatio;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(-viewWidth / 2.0f, viewWidth / 2.0f);
    std::uniform_real_distribution<float> yDist(-viewHeight / 2.0f, viewHeight / 2.0f);
    std::uniform_real_distribution<float> freqDist(2.0f, 5.0f);
    std::uniform_real_distribution<float> scaleDist(3.0f, 7.0f);

    const int kNumParticleSystems = 5;
    for (int i = 0; i < kNumParticleSystems; ++i) {
        auto particleSystem = std::make_unique<ParticleSystem>();
        particleSystem->Initialize(camera_.get(), "resources/gradationLine.png", ParticleType::Normal, ParticlePrimitiveShape::Ring);
        
        // 画面内にランダムに配置
        particleSystem->SetEmitterPosition({ xDist(gen), yDist(gen), 0.0f });
        
        particleSystem->SetEmitterArea({ 0.0f, 0.0f, 0.0f });
        particleSystem->SetEmitterCount(1);
        particleSystem->SetEmitterFrequency(freqDist(gen));
        
        float startScale = 0.1f;
        float endScale = scaleDist(gen);
        particleSystem->SetParticleScale({ startScale, startScale, 1.0f }, { endScale, endScale, 1.0f });
        
        particleSystem->SetParticleColor({ 1.0f, 1.0f, 1.0f, 0.5f }, { 1.0f, 1.0f, 1.0f, 0.0f });
        particleSystem->SetRingParameters(0.8f, 1.0f, 0.0f, 360.0f, 32);
        
        particleSystems_.push_back(std::move(particleSystem));
    }
}

// 更新
void TitleScene::Update() {


#if defined USE_IMGUI

    ImGui::Begin("TitleScene");
    if (ImGui::BeginTabBar("TitleSceneTabs")) {

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
            int i = 0;
            for (auto& ps : particleSystems_) {
                std::string name = "Particle " + std::to_string(i++);
                ps->Debug(name.c_str());
            }
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

    UpdateTextAnimation();

#pragma region takamura追加
    // キー入力でトランジション開始
    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
        if (!isTransitioning) {
            isTransitioning = true;
            isTextAnimationFast_ = true; // アニメーションを高速化
            seDecision_.Play(false);
            stripeTransition_->Start();
        }
    }

    // トランジション更新
    if (isTransitioning) {
        stripeTransition_->Update();

        if (stripeTransition_->IsFinished()) {
            engine_->GetSceneManager()->Request("InGame");
        }
    }
#pragma endregion takamura追加
    // タイトルモデルの更新
    if (titleObj_) {
        titleObj_->Update();
    }

    // 背景パーティクルの更新
    for (auto& ps : particleSystems_) {
        ps->Update();
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

void TitleScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    
    engine_->ApplyPSO();

    // タイトルモデルを描画
    if (titleObj_) {
        titleObj_->Draw();
    }

    // 背景パーティクルの描画
    for (auto& ps : particleSystems_) {
        ps->Draw();
    }

    // 2D

    engine_->ApplySpritePSO();
    // テキストスプライト描画
    if (textSprite_pushStart_) {
        textSprite_pushStart_->Draw();
    }

#pragma region takamura追加
    stripeTransition_->Draw();
#pragma endregion takamura追加

}

void TitleScene::UpdateTextAnimation() {
    if (!textSprite_pushStart_) return;

    // アニメーション速度を決定
    int animationSpeed = isTextAnimationFast_ ? 4 : 1;
    textAnimationTimer_ = (textAnimationTimer_ + animationSpeed) % 120;

    // sin波を使ってアルファ値を計算 (0.5から1.0の範囲で変動)
    float sine = std::sin(static_cast<float>(textAnimationTimer_) * 3.14159265f / 60.0f);
    float alpha = 0.75f + 0.25f * sine;

    textSprite_pushStart_->SetAlpha(alpha);
}