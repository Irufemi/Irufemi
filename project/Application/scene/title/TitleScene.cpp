#include "TitleScene.h"

#include "Framework/SceneManager.h"
#include <cmath>

#include "Irufemi.h"

#include "Engine/Graphics/Camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/AreaLight.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"

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

    // --- 3Dタイトル文字の初期化と配置 ---
    // 上段：「七転び」 (Y = 2.0, 少し左寄り)
    titleTextNana_ = std::make_unique<ObjClass>();
    titleTextNana_->Initialize(camera_.get(), "title/text/text_nana.obj");
    titleTextNana_->SetPosition({ -3.5f, 2.0f, 0.0f });
    titleTextNana_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextKoro1_ = std::make_unique<ObjClass>();
    titleTextKoro1_->Initialize(camera_.get(), "title/text/text_koro.obj");
    titleTextKoro1_->SetPosition({ -0.5f, 2.0f, 0.0f });
    titleTextKoro1_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextBi1_ = std::make_unique<ObjClass>();
    titleTextBi1_->Initialize(camera_.get(), "title/text/text_bi.obj");
    titleTextBi1_->SetPosition({ 2.5f, 2.0f, 0.0f });
    titleTextBi1_->SetScale({ 1.5f, 1.5f, 1.5f });

    // 下段：「八転び」 (Y = 0.0, 少し右寄り)
    titleTextHati_ = std::make_unique<ObjClass>();
    titleTextHati_->Initialize(camera_.get(), "title/text/text_hati.obj");
    titleTextHati_->SetPosition({ -2.5f, 0.0f, 0.0f });
    titleTextHati_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextKoro2_ = std::make_unique<ObjClass>();
    titleTextKoro2_->Initialize(camera_.get(), "title/text/text_koro.obj");
    titleTextKoro2_->SetPosition({ 0.5f, 0.0f, 0.0f });
    titleTextKoro2_->SetScale({ 1.5f, 1.5f, 1.5f });

    titleTextBi2_ = std::make_unique<ObjClass>();
    titleTextBi2_->Initialize(camera_.get(), "title/text/text_bi.obj");
    titleTextBi2_->SetPosition({ 3.5f, 0.0f, 0.0f });
    titleTextBi2_->SetScale({ 1.5f, 1.5f, 1.5f });

    // 「Push to Space」文字
    titleTextPushToSpace_ = std::make_unique<ObjClass>();
    titleTextPushToSpace_->Initialize(camera_.get(), "text_pushtospace/text_pushtospace.obj");
    titleTextPushToSpace_->SetPosition({ 0.0f, -2.5f, 0.0f });
    titleTextPushToSpace_->SetScale({ 1.0f, 1.0f, 1.0f });
    
}

// 更新
void TitleScene::Update() {


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
        camera_->Update();
    }

    // =====
    // ↓ゲームの更新
    // =====

    // 3D文字のアニメーション（ふわふわ浮遊）
    animationTime_ += 1.0f / 60.0f;
    const float baseYTop = 2.0f;
    const float baseYBottom = 0.0f;
    const float basePositionsX[6] = { -3.5f, -0.5f, 2.5f, -2.5f, 0.5f, 3.5f };

    ObjClass* texts[6] = {
        titleTextNana_.get(), titleTextKoro1_.get(), titleTextBi1_.get(),
        titleTextHati_.get(), titleTextKoro2_.get(), titleTextBi2_.get()
    };

    for (int i = 0; i < 6; ++i) {
        if (!texts[i]) continue;

        // 文字ごとに位相（タイミング）をずらす
        float phase = animationTime_ * 2.0f + i * 0.5f;

        // 1. ふわふわ浮遊 (Y軸の上下動)
        float offsetY = std::sin(phase) * 0.2f;
        float baseY = (i < 3) ? baseYTop : baseYBottom;
        texts[i]->SetPosition({ basePositionsX[i], baseY + offsetY, 0.0f });

        // 2. 回転はさせず、常に正面を向かせる
        texts[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
    }

    // 「Push to Space」文字の明滅（ダークソウル風）
    if (titleTextPushToSpace_) {
        float alpha = 1.0f;
        isDrawPushToSpace_ = true; // デフォルトは描画する

        if (!isChangingScene_) {
            // 待機中：ゆっくりとした明滅（0.2 〜 1.0 の間をサイン波で推移）
            alpha = 0.6f + std::sin(animationTime_ * 3.0f) * 0.4f;
        } else {
            // 決定後：適度な速さのフラッシュ
            // 注: ObjClass内部でAlpha 0.0 が 1.0 にクランプされる仕様があるため、
            // Alphaではなく描画自体をスキップすることで明滅を表現する
            isDrawPushToSpace_ = (std::sin(animationTime_ * 40.0f) > 0.0f);
        }
        titleTextPushToSpace_->SetAlpha(alpha);
    }

    // SPACEキーを押したらシーン遷移のフラグを立てる（まだ遷移開始しない）
    if (!isChangingScene_ && engine_->GetInputManager()->IsKeyPressed(VK_SPACE)) {
        isChangingScene_ = true;
        transitionDelayTimer_ = 0.0f;
    }

    // フラグが立っていればタイマーを進め、一定時間後にシーン遷移リクエストを送る
    if (isChangingScene_ && !isTransitionRequested_) {
        transitionDelayTimer_ += 1.0f / 60.0f;
        
        // 0.8秒間チカチカ点滅を見せた後に実際の遷移を開始する
        if (transitionDelayTimer_ >= 0.8f) {
            engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::Slide, 1.0f);
            isTransitionRequested_ = true;
        }
    }


    // =====
    // ↑ゲームの更新
    // =====

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

void TitleScene::Draw() {

    // --- 3Dタイトル文字描画 ---
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    if (titleTextNana_) titleTextNana_->Draw();
    if (titleTextKoro1_) titleTextKoro1_->Draw();
    if (titleTextBi1_) titleTextBi1_->Draw();
    if (titleTextHati_) titleTextHati_->Draw();
    if (titleTextKoro2_) titleTextKoro2_->Draw();
    if (titleTextBi2_) titleTextBi2_->Draw();
    if (titleTextPushToSpace_ && isDrawPushToSpace_) titleTextPushToSpace_->Draw();

}

void TitleScene::DrawDebugTab() {
#if defined USE_IMGUI
    if (camera_) {
        if (ImGui::BeginTabItem("Main Camera")) {
            ImGui::Checkbox("Debug Camera Mode", &debugMode_);
            if (debugMode_ && debugCamera_) {
                if (ImGui::Button("Top-Down")) debugCamera_->SetPreset(DebugCamera::Preset::TopDown, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Diagonal")) debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Front")) debugCamera_->SetPreset(DebugCamera::Preset::Front, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Snap to Current")) debugCamera_->SetPreset(DebugCamera::Preset::Current, *camera_);

                ImGui::Separator();
                ImGui::Text("Debug Camera Controls");
                debugCamera_->GetCamera().DrawDebugContents();
                float dist = debugCamera_->GetDistance();
                if (ImGui::DragFloat("Orbit Distance", &dist, 0.1f, 1.0f, 1000.0f)) {
                    debugCamera_->SetDistance(dist);
                }
            } else {
                camera_->DrawDebugContents();
            }
            ImGui::EndTabItem();
        }
    }
    DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);

    // Texture タブ
    if (ImGui::BeginTabItem("Texture")) {
        if (ImGui::Button("allLoadActivate")) {
            engine_->GetTextureManager()->LoadAllFromFolder("resources/");
        }
        ImGui::EndTabItem();
    }
#endif
}
