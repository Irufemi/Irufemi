#include "GameOverScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/AreaLight.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"

GameOverScene::~GameOverScene() {

}

void GameOverScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // カメラ(2D 正射影)
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
    camera_->UpdateMatrix();

    // デバッグカメラの初期化を追加
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

    // 「GameOver...」文字の初期化
    goTextG_ = std::make_unique<ObjClass>();
    goTextG_->Initialize(camera_.get(), "gameOver/text_G.obj");
    goTextA_ = std::make_unique<ObjClass>();
    goTextA_->Initialize(camera_.get(), "gameOver/text_q.obj"); // ※ファイル名text_q.objはaとして使用
    goTextM_ = std::make_unique<ObjClass>();
    goTextM_->Initialize(camera_.get(), "gameOver/text_m.obj");
    goTextE1_ = std::make_unique<ObjClass>();
    goTextE1_->Initialize(camera_.get(), "gameOver/text_e.obj");
    goTextO_ = std::make_unique<ObjClass>();
    goTextO_->Initialize(camera_.get(), "gameOver/text_O.obj");
    goTextV_ = std::make_unique<ObjClass>();
    goTextV_->Initialize(camera_.get(), "gameOver/text_v.obj");
    goTextE2_ = std::make_unique<ObjClass>();
    goTextE2_->Initialize(camera_.get(), "gameOver/text_e.obj");
    goTextR_ = std::make_unique<ObjClass>();
    goTextR_->Initialize(camera_.get(), "gameOver/text_r.obj");
    
    // text_dot.obj（...）の読み込み
    goTextDot_ = std::make_unique<ObjClass>();
    goTextDot_->Initialize(camera_.get(), "gameOver/text_dot.obj");

    // 「Push to Space」文字の初期化
    textPushToSpace_ = std::make_unique<ObjClass>();
    textPushToSpace_->Initialize(camera_.get(), "text_pushtospace/text_pushtospace.obj");
    textPushToSpace_->SetPosition({ 0.0f, -2.5f, 0.0f });
    textPushToSpace_->SetScale({ 1.0f, 1.0f, 1.0f });
}

void GameOverScene::Update() {

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

    // GameOver文字のアニメーション（重苦しいゆっくりとした浮遊）
    const float goBaseY = 1.0f;
    const float goPositionsX[9] = {
        -5.0f, -3.8f, -2.4f, -1.2f, // Game
         0.4f,  1.6f,  2.8f,  4.0f, // Over
         5.5f                       // ...
    };

    ObjClass* goTexts[9] = {
        goTextG_.get(), goTextA_.get(), goTextM_.get(), goTextE1_.get(),
        goTextO_.get(), goTextV_.get(), goTextE2_.get(), goTextR_.get(),
        goTextDot_.get()
    };

    for (int i = 0; i < 9; ++i) {
        if (!goTexts[i]) continue;
        
        // 重々しいゆっくりとした動き
        float phase = animationTime_ * 1.5f + i * 0.3f;
        float offsetY = std::sin(phase) * 0.3f;
        
        goTexts[i]->SetPosition({ goPositionsX[i], goBaseY + offsetY, 0.0f });
        goTexts[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
        goTexts[i]->SetScale({ 1.2f, 1.2f, 1.2f });
    }

    // 「Push to Space」文字の明滅
    if (textPushToSpace_) {
        animationTime_ += 1.0f / 60.0f;
        float alpha = 1.0f;
        isDrawPushToSpace_ = true;

        if (!isChangingScene_) {
            // 待機中：ゆっくりとした明滅
            alpha = 0.6f + std::sin(animationTime_ * 3.0f) * 0.4f;
        } else {
            // 決定後：高速フラッシュ
            isDrawPushToSpace_ = (std::sin(animationTime_ * 40.0f) > 0.0f);
        }
        textPushToSpace_->SetAlpha(alpha);
    }

    // Spaceキーが押されたらフラグを立てる
    if (!isChangingScene_ && engine_->GetInputManager()->IsKeyPressed(VK_SPACE)) {
        isChangingScene_ = true;
        transitionDelayTimer_ = 0.0f;
    }

    // 遷移フラグが立っていればタイマーを進める
    if (isChangingScene_ && !isTransitionRequested_) {
        transitionDelayTimer_ += 1.0f / 60.0f;
        if (transitionDelayTimer_ >= 0.8f) {
            engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
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

void GameOverScene::Draw() {

    // 2Dスプライト描画処理を削除し、直接3Dモデルの描画へ移行

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);
    engine_->ApplyPSO();

    if (goTextG_) goTextG_->Draw();
    if (goTextA_) goTextA_->Draw();
    if (goTextM_) goTextM_->Draw();
    if (goTextE1_) goTextE1_->Draw();
    if (goTextO_) goTextO_->Draw();
    if (goTextV_) goTextV_->Draw();
    if (goTextE2_) goTextE2_->Draw();
    if (goTextR_) goTextR_->Draw();
    if (goTextDot_) goTextDot_->Draw();

    if (textPushToSpace_ && isDrawPushToSpace_) {
        textPushToSpace_->Draw();
    }
}

void GameOverScene::DrawDebugTab() {
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


