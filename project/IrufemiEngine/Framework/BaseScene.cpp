#include "BaseScene.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Graphics/Camera/DebugCamera.h"
#include "Engine/Graphics/Data/CameraForGPU.h"
#include "Engine/Graphics/Data/PointLight.h"
#include "Engine/Graphics/Data/SpotLight.h"
#include "Engine/Graphics/Data/DirectionalLight.h"
#include "Engine/Graphics/Data/AreaLight.h"
#include "GameObject.h"
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include "Engine/Manager/DebugUI.h"
#endif

BaseScene::BaseScene() = default;
BaseScene::~BaseScene() = default;

void BaseScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // --- カメラマネージャーの初期化はエンジン側で行われるため、ここではメインカメラの登録のみ行う ---
    auto mainCamera = std::make_shared<Camera>();
    mainCamera->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    mainCamera->SetTranslate({ 0.0f, 0.0f, -50.0f });
    mainCamera->UpdateMatrix();
    engine_->GetCameraManager()->AddCamera("Main", mainCamera);

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());

    // --- デフォルトライティングの初期化 ---
    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLight_->direction = { 0.5f, -0.7f, 1.0f };
    directionalLight_->intensity = 1.0f;
}

void BaseScene::Update() {
    // デバッグカメラのトグル機能などをここに入れることも可能
    // 今回は各シーンが個別に実装しているケースを考慮し、Updateでのカメラ行列上書き処理を共通化
    if (isDebugCameraMode_) {
        debugCamera_->Update();
        Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
        if (activeCam) {
            const Camera& dbgCam = debugCamera_->GetCamera();
            activeCam->SetViewMatrix(dbgCam.GetViewMatrix());
            activeCam->SetTranslate(dbgCam.GetTranslate());
            activeCam->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
        }
    } else {
        engine_->GetCameraManager()->Update();
    }

    // GameObject の更新
    for (auto& obj : gameObjects_) {
        if (obj) obj->Update();
    }

    SubmitFrameData();
}

void BaseScene::Draw() {
    // GameObject の描画
    for (auto& obj : gameObjects_) {
        if (obj) obj->Draw();
    }
}

void BaseScene::AddGameObject(std::shared_ptr<GameObject> obj) {
    if (obj) {
        gameObjects_.push_back(obj);
    }
}

void BaseScene::RemoveGameObject(std::shared_ptr<GameObject> obj) {
    auto it = std::find(gameObjects_.begin(), gameObjects_.end(), obj);
    if (it != gameObjects_.end()) {
        gameObjects_.erase(it);
    }
}

void BaseScene::SubmitFrameData() {
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    CameraForGPU cameraForGpu;
    cameraForGpu.view = activeCam->GetViewMatrix();
    cameraForGpu.projection = activeCam->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = activeCam->GetTranslate();

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

    if (directionalLight_) {
        engine_->GetDrawManager()->SetFrameData(cameraForGpu, engine_->GetTotalTime(), engine_->GetDeltaTime(), *directionalLight_, pLights, sLights, aLights);
    }
}

void BaseScene::DrawDebugTab() {
#ifdef USE_IMGUI
    if (ImGui::BeginTabItem("Camera & Lights")) {
        ImGui::Checkbox("Debug Camera Mode", &isDebugCameraMode_);
        if (isDebugCameraMode_ && debugCamera_) {
            Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
            if (activeCam) {
                if (ImGui::Button("Top-Down")) debugCamera_->SetPreset(DebugCamera::Preset::TopDown, *activeCam);
                ImGui::SameLine();
                if (ImGui::Button("Diagonal")) debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *activeCam);
                ImGui::SameLine();
                if (ImGui::Button("Front")) debugCamera_->SetPreset(DebugCamera::Preset::Front, *activeCam);
                ImGui::SameLine();
                if (ImGui::Button("Snap to Current")) debugCamera_->SetPreset(DebugCamera::Preset::Current, *activeCam);
            }
            ImGui::Separator();
            ImGui::Text("Debug Camera Controls");
            debugCamera_->GetCamera().DrawDebugContents();
            float dist = debugCamera_->GetDistance();
            if (ImGui::DragFloat("Orbit Distance", &dist, 0.1f, 1.0f, 1000.0f)) {
                debugCamera_->SetDistance(dist);
            }
        } else {
            Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
            if (activeCam) {
                activeCam->DrawDebugContents();
            }
        }
        ImGui::EndTabItem();
    }
    DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);
#endif
}

// ── 入力ヘルパ ──
bool BaseScene::DownVK(uint8_t vk) const { return engine_->GetInputManager()->IsKeyDown(vk); }
bool BaseScene::PressedVK(uint8_t vk) const { return engine_->GetInputManager()->IsKeyPressed(vk); }
bool BaseScene::ReleasedVK(uint8_t vk) const { return engine_->GetInputManager()->IsKeyReleased(vk); }

bool BaseScene::DownDIK(uint8_t dik) const { return engine_->GetInputManager()->IsKeyDownDIK(dik); }
bool BaseScene::PressedDIK(uint8_t dik) const { return engine_->GetInputManager()->IsKeyPressedDIK(dik); }
bool BaseScene::ReleasedDIK(uint8_t dik) const { return engine_->GetInputManager()->IsKeyReleasedDIK(dik); }

bool BaseScene::IsButtonDown(unsigned short button) const { return engine_->GetInputManager()->IsButtonDown(button); }
bool BaseScene::IsButtonPressed(unsigned short button) const { return engine_->GetInputManager()->IsButtonPressed(button); }

void BaseScene::SaveScene(const std::string& filepath) {
    nlohmann::json rootArray = nlohmann::json::array();
    for (const auto& obj : gameObjects_) {
        rootArray.push_back(obj->Serialize());
    }

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << rootArray.dump(4);
        file.close();
    }
}

void BaseScene::LoadScene(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    nlohmann::json rootArray;
    file >> rootArray;
    file.close();

    gameObjects_.clear();

    if (rootArray.is_array()) {
        for (const auto& j : rootArray) {
            auto obj = std::make_shared<GameObject>();
            obj->Deserialize(j);
            obj->Initialize();
            AddGameObject(obj);
        }
    }
}
