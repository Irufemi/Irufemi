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

    /// Sprite
    // テキスト
    // 血管壊回
    textSprite_title_ = std::make_unique<Sprite>();
    textSprite_title_->Initialize(camera_.get(), "resources/texture/text_title.png");
    // 押したらスタート
    textSprite_pushStart_ = std::make_unique<Sprite>();
    textSprite_pushStart_->Initialize(camera_.get(), "resources/texture/text_title.png");

#pragma region takamura追加
    float screenHeight = static_cast<float>(engine_->GetClientHeight());
    float offScreenY = screenHeight + 300.0f;
    float spacing = static_cast<float>(engine_->GetClientWidth()) / transitionStripeIndex;
    const std::string& texturePath = "resources/texture/stripe.png";

    for (int i = 0; i < transitionStripeIndex; ++i) {
        auto stripe = std::make_unique<Sprite>();

        stripe->Initialize(camera_.get(), texturePath);
        stripe->SetSize(stripeWidth, stripeHeight);

        // 初期位置（画面外の上）
        float x = spacing * i - 50.0f;
        float y = -offScreenY;
        stripe->SetPosition(x, y);

        stripeSprites_.push_back(std::move(stripe));
        stripeProgress_.push_back(0.0f);

        for (int i = 0; i < static_cast<int>(stripeSprites_.size()); ++i) {
            Vector2 pos = stripeSprites_[i]->GetPosition2D();
            Vector2 size = stripeSprites_[i]->GetSize();
            char buf[256];
            sprintf_s(buf, "Stripe[%d] after init: pos=(%.1f, %.1f), size=(%.1f, %.1f)\n",
                i, pos.x, pos.y, size.x, size.y);
            OutputDebugStringA(buf);
        }
    }
#pragma endregion takamura追加
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
            ImGui::EndTabItem();
        }

        // Transition タブ
        if (ImGui::BeginTabItem("Transition")) {
            ImGui::Text("isTransitioning: %s", isTransitioning ? "true" : "false");
            ImGui::Text("transitionTimer: %d", transitionTimer);
            ImGui::Text("stripeSprites count: %zu", stripeSprites_.size());
            ImGui::Text("stripeProgress count: %zu", stripeProgress_.size());

            ImGui::Separator();

            // 各ストライプの情報
            for (int i = 0; i < transitionStripeIndex && i < static_cast<int>(stripeSprites_.size()); ++i) {
                Vector2 pos = stripeSprites_[i]->GetPosition2D();
                Vector2 size = stripeSprites_[i]->GetSize();
                ImGui::Text("Stripe[%d]: pos=(%.1f, %.1f), size=(%.1f, %.1f), progress=%.2f",
                    i, pos.x, pos.y, size.x, size.y, stripeProgress_[i]);
            }

            ImGui::Separator();

            // 手動テストボタン
            if (ImGui::Button("Start Transition")) {
                isTransitioning = true;
                transitionTimer = 0;
                for (auto& p : stripeProgress_) {
                    p = 0.0f;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset")) {
                isTransitioning = false;
                transitionTimer = 0;
                for (auto& p : stripeProgress_) {
                    p = 0.0f;
                }
            }

            // 強制表示ボタン
            if (ImGui::Button("Force Show All (progress=1)")) {
                for (auto& p : stripeProgress_) {
                    p = 1.0f;
                }
                // 位置も更新
                float screenWidth = static_cast<float>(engine_->GetClientWidth());
                float spacing = screenWidth / transitionStripeIndex;
                for (int i = 0; i < transitionStripeIndex && i < static_cast<int>(stripeSprites_.size()); ++i) {
                    float x = spacing * i - 100.0f;
                    float y = 0.0f;
                    stripeSprites_[i]->SetPosition(x, y);
                }
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

    // なんかのキー入力
    if (engine_->GetInputManager()->IsKeyDownDIK(0x39)) {
        // シーン切り替え開始
        if (!isTransitioning) {
            isTransitioning = true;
        }
    }

    if (isTransitioning) {
        ++transitionTimer;

        float screenWidth = static_cast<float>(engine_->GetClientWidth());
        float screenHeight = static_cast<float>(engine_->GetClientHeight());
        float spacing = screenWidth / transitionStripeIndex;
        float spacingOffset = 50.0f;

        // トリガー位置：前のスプライトの下端が、自身の高さの半分の位置に来たら
        float triggerY = stripeHeight / 2.0f;

        for (int i = 0; i < transitionStripeIndex; ++i) {

            // 最初のスプライトは即開始、それ以外は前のスプライトの位置で判定
            bool shouldStart = false;

            if (i == 0) {
                // 最初のスプライトは即開始
                shouldStart = true;
            } else {
                // 前のスプライトの下端位置を計算
                Vector2 prevPos = stripeSprites_[i - 1]->GetPosition2D();
                float prevBottom = prevPos.y + stripeHeight;  // 下端 = Y + 高さ

                // 前のスプライトの下端の1/3の位置
                float prevTriggerPoint = prevPos.y + (stripeHeight * 2.0f / 3.0f);

                // 自身の高さの半分の位置に達したら開始
                if (prevTriggerPoint >= triggerY) {
                    shouldStart = true;
                }
            }

            // 降下処理
            if (shouldStart && stripeProgress_[i] < 1.0f) {
                float moveSpeed = 0.1f;  // 大きいほど速い（0.01〜1.0くらいで調整）
                stripeProgress_[i] += moveSpeed;
                if (stripeProgress_[i] > 1.0f) {
                    stripeProgress_[i] = 1.0f;
                }
            }

            // 初期位置
            float startX = (spacing + spacingOffset) * i + 50.0f;
            float startY = -screenHeight - 200.0f;

            // 最終位置
            float endX = (spacing + spacingOffset) * i - 450.0f;
            float endY = -50.0f;

            // 線形補間
            float x = startX + (endX - startX) * stripeProgress_[i];
            float y = startY + (endY - startY) * stripeProgress_[i];

            stripeSprites_[i]->SetPosition(x, y);
        }
    }

    if (transitionTimer >= transitionTime) {
        // シーンの切り替え
        engine_->GetSceneManager()->Request("InGame");
    }

    for (auto& stripe : stripeSprites_) {
        stripe->Update();
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
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // 逆順で描画してみる
    for (int i = transitionStripeIndex - 1; i >= 0; --i) {
        if (stripeProgress_[i] > 0.0f) {
            stripeSprites_[i]->Draw();
        }
    }
}