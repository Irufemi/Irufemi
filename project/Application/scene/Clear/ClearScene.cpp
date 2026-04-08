#include "ClearScene.h"

#include "Framework/SceneManager.h"

#include "Irufemi.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/AreaLight.h"

ClearScene::~ClearScene() {

}

void ClearScene::Initialize(IrufemiEngine* engine) {
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

    // 背景スプライトの初期化
    backSprite_ = std::make_unique<Sprite>();
    backSprite_->Initialize(camera_.get(), "resources/whiteTexture.png");
    backSprite_->SetSize(static_cast<float>(engine_->GetClientWidth()), static_cast<float>(engine_->GetClientHeight()));
    backSprite_->SetPosition(0.0f, 0.0f);
    backSprite_->SetColor(Vector4{ 95.0f / 255.0f,205.0f / 255.0f,228.0f / 255.0f,1.0f });
    backSprite_->Update();

    // シーン表示仮置きスプライトの初期化
    sampleSprite_ = std::make_unique<Sprite>();
    sampleSprite_->Initialize(camera_.get(), "resources/texture/clear/clear.png");

}

void ClearScene::Update() {

    // --- カメラの更新 ---

    if (debugMode_) {
        // デバッグカメラを更新
        debugCamera_->Update();
        // デバッグカメラの計算結果をメインカメラに上書きする
        const Camera& dbgCam = debugCamera_->GetCamera();
        camera_->SetViewMatrix(dbgCam.GetViewMatrix());
        camera_->SetTranslate(dbgCam.GetTranslate());
        camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
    } else {
        // 通常カメラの更新
        camera_->Update();
    }

    // =====
    // ↓ゲームの更新
    // =====

    // Spaceキーが押されていたらタイトルへ演出付きで遷移（1.0秒）
    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE)) {
        engine_->GetSceneManager()->TransitionTo("Title", SceneTransition::Type::Fade, 1.0f);
    }

    // シーン表示仮置きスプライトの更新
    sampleSprite_->Update();

    // 背景スプライトの更新
    backSprite_->Update();

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

void ClearScene::Draw() {

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplySpritePSO();

    // 背景スプライトの描画
    backSprite_->Draw();

    // シーン表示仮置きスプライトの描画
    sampleSprite_->Draw();

}

void ClearScene::DrawDebugTab() {
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


