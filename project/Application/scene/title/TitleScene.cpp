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

    // 文字の初期化
    const float textScale = 1.5f;
    const float textSpacing = 1.5f; // 文字の間隔
    const int numTexts = 5;
    const float totalWidth = textSpacing * (numTexts - 1);
    const float startX = -totalWidth / 2.0f;
    const float startY = 2.0f; // Y軸の位置を調整

    initialTextPositions_.resize(numTexts);

    // タイトル(アンナイトのア)
    text_a_ = std::make_unique<ObjClass>();
    text_a_->Initialize(camera_.get(), "resources/obj/titleText_a.obj");
    initialTextPositions_[0] = { startX + textSpacing * 0, startY, 0.0f };
    text_a_->SetPosition(initialTextPositions_[0]);
    text_a_->SetScale({ textScale, textScale, textScale });
    text_a_->SetColor(Vector4{ 99.0f / 255.0f,39.0f / 255.0f,178.0f / 255.0f,1.0f });

    // タイトル(アンナイトのン)
    text_n_ = std::make_unique<ObjClass>();
    text_n_->Initialize(camera_.get(), "resources/obj/titleText_n.obj");
    initialTextPositions_[1] = { startX + textSpacing * 1, startY, 0.0f };
    text_n_->SetPosition(initialTextPositions_[1]);
    text_n_->SetScale({ textScale, textScale, textScale });
    text_n_->SetColor(Vector4{ 99.0f / 255.0f,39.0f / 255.0f,178.0f / 255.0f,1.0f });

    // タイトル(アンナイトのナ)
    text_na_ = std::make_unique<ObjClass>();
    text_na_->Initialize(camera_.get(), "resources/obj/titleText_na.obj");
    initialTextPositions_[2] = { startX + textSpacing * 2, startY, 0.0f };
    text_na_->SetPosition(initialTextPositions_[2]);
    text_na_->SetScale({ textScale, textScale, textScale });
    text_na_->SetColor(Vector4{ 99.0f / 255.0f,39.0f / 255.0f,178.0f / 255.0f,1.0f });

    // タイトル(アンナイトのイ)
    text_i_ = std::make_unique<ObjClass>();
    text_i_->Initialize(camera_.get(), "resources/obj/titleText_i.obj");
    initialTextPositions_[3] = { startX + textSpacing * 3, startY, 0.0f };
    text_i_->SetPosition(initialTextPositions_[3]);
    text_i_->SetScale({ textScale, textScale, textScale });
    text_i_->SetColor(Vector4{ 99.0f / 255.0f,39.0f / 255.0f,178.0f / 255.0f,1.0f });

    // タイトル(アンナイトのト)
    text_to_ = std::make_unique<ObjClass>();
    text_to_->Initialize(camera_.get(), "resources/obj/titleText_to.obj");
    initialTextPositions_[4] = { startX + textSpacing * 4, startY, 0.0f };
    text_to_->SetPosition(initialTextPositions_[4]);
    text_to_->SetScale({ textScale, textScale, textScale });
    text_to_->SetColor(Vector4{ 99.0f / 255.0f,39.0f / 255.0f,178.0f / 255.0f,1.0f });

    // プッシュキー
    text_pushKey_ = std::make_unique<Sprite>();
    text_pushKey_->Initialize(camera_.get(), "resources/texture/titleText_pushKey.png");
    text_pushKey_->SetAnchor(0.5f, 0.5f);
    text_pushKey_->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f + 150.0f);

    // フェードの初期化
    fade_ = std::make_unique<Fade>();
    fade_->Initialize(camera_.get());

    // bgm
    bgm_ = std::make_unique<Bgm>();
    bgm_->Initialize("resources/bgm/title.mp3");
    bgm_->PlayFixed();
    // se(決定音)
    se_select_ = std::make_unique<Se>();
    se_select_->Initialize("resources/se/se_select.mp3");
}

// 更新
void TitleScene::Update() {


#if defined USE_IMGUI

    ImGui::Begin("TitleScene");
    if (ImGui::BeginTabBar("TitleSceneTabs")) {

        DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_);

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

    // --- 文字のアニメーション ---
    animationTimer_ += 1.0f / 60.0f; // 60FPSを想定
    const float floatOffset = std::sin(animationTimer_ * floatSpeed_) * floatAmplitude_;

    if (text_a_) {
        Vector3 newPos = initialTextPositions_[0];
        newPos.y += floatOffset;
        text_a_->SetPosition(newPos);
        text_a_->Update();
    }
    if (text_n_) {
        Vector3 newPos = initialTextPositions_[1];
        newPos.y += std::sin(animationTimer_ * floatSpeed_ + 0.5f) * floatAmplitude_; // 少しずらす
        text_n_->SetPosition(newPos);
        text_n_->Update();
    }
    if (text_na_) {
        Vector3 newPos = initialTextPositions_[2];
        newPos.y += std::sin(animationTimer_ * floatSpeed_ + 1.0f) * floatAmplitude_;
        text_na_->SetPosition(newPos);
        text_na_->Update();
    }
    if (text_i_) {
        Vector3 newPos = initialTextPositions_[3];
        newPos.y += std::sin(animationTimer_ * floatSpeed_ + 1.5f) * floatAmplitude_;
        text_i_->SetPosition(newPos);
        text_i_->Update();
    }
    if (text_to_) {
        Vector3 newPos = initialTextPositions_[4];
        newPos.y += std::sin(animationTimer_ * floatSpeed_ + 2.0f) * floatAmplitude_;
        text_to_->SetPosition(newPos);
        text_to_->Update();
    }

    // --- プッシュキーのアニメーション ---
    pushKeyAnimTimer_ += 1.0f / 60.0f;
    Vector4 color = text_pushKey_->GetColor();

    switch (pushKeyState_) {
    case PushKeyState::NormalBlink:
    {
        // ゆっくり明滅
        const float normalBlinkSpeed = 2.0f;
        color.w = 0.6f + 0.4f * std::abs(std::sin(pushKeyAnimTimer_ * normalBlinkSpeed));
        text_pushKey_->SetColor(color);

        // 入力で高速明滅へ移行
        if (!isChangingScene_ && (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A))) {
            se_select_->Play();
            pushKeyState_ = PushKeyState::FastBlink;
            pushKeyAnimTimer_ = 0.0f; // タイマーリセット
        }
    }
    break;

    case PushKeyState::FastBlink:
    {
        // 高速明滅
        const float fastBlinkSpeed = 15.0f;
        const float fastBlinkDuration = 0.6f;
        color.w = 0.5f + 0.5f * std::abs(std::sin(pushKeyAnimTimer_ * fastBlinkSpeed));
        text_pushKey_->SetColor(color);

        // 一定時間後にフェードアウト開始
        if (pushKeyAnimTimer_ > fastBlinkDuration) {
            pushKeyState_ = PushKeyState::Done;
            color.w = 0.0f; // 非表示に
            text_pushKey_->SetColor(color);

            // 1秒かけて黒にフェードアウト
            fade_->FadeOut(1.0f, { 0.0f, 0.0f, 0.0f, 1.0f });
            isChangingScene_ = true;
        }
    }
    break;

    case PushKeyState::Done:
        // 何もしない
        break;
    }

    text_pushKey_->Update();


    // フェード処理の更新
    fade_->Update();

    // フェードアウトが完了したらシーン遷移をリクエスト
    if (isChangingScene_ && fade_->IsDone()) {
        engine_->GetSceneManager()->Request("Select");
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

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights);
}

void TitleScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    if (text_a_) { text_a_->Draw(); }
    if (text_n_) { text_n_->Draw(); }
    if (text_na_) { text_na_->Draw(); }
    if (text_i_) { text_i_->Draw(); }
    if (text_to_) { text_to_->Draw(); }

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    text_pushKey_->Draw();

    // フェードの描画
    fade_->Draw();
}