#include "GameScene.h"

#include "actors/player/Player.h" 
#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/CameraForGPU.h"
#include "actors/enemy/Enemy.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "math/AreaLight.h"
#include "2D/Sprite.h"
#include "contents/field/Field.h"

GameScene::GameScene() {}

GameScene::~GameScene() {
}

void GameScene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

    // プレイヤーの初期化
    player_ = std::make_unique<Player>();
    player_->Initialize(engine_->GetInputManager(), camera_.get(), engine_);

    boss_ = std::make_unique<Enemy>();
    boss_->Initialize(camera_.get());

    field_ = std::make_unique<Field>(camera_.get(), engine_);
    field_->Initialize();

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLight_->direction = { 0.0f, -1.0f, 1.0f };
    directionalLight_->intensity = 1.0f;
}

// 更新
void GameScene::Update() {

#ifdef USE_IMGUI
    ImGui::Begin("Debug");
    ImGui::Checkbox("Debug Camera", &debugMode_);
    ImGui::End();
#endif

    // =====
    // ↓ゲームの更新
    // =====


    // プレイヤーの更新（今と同じく、デバッグカメラ中はプレイヤーの動きを止める）
    if (player_ && !debugMode_) {
        player_->Update();
    }

    if (boss_) {
        boss_->Update();
    }

    // =====
    // ↑ゲームの更新
    // =====


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
        // 通常カメラの更新（プレイヤーのカメラ位置を反映する）
        camera_->Update("Main Camera");
    }


    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = camera_->GetViewMatrix();
    cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = camera_->GetTranslate();

    std::vector<PointLight*> pLights;
    std::vector<SpotLight*> sLights;
    std::vector<AreaLight*> aLights;

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);
}

// 描画
void GameScene::Draw() {

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);
    engine_->ApplyPSO();

    if (field_) {
        field_->Draw();
    }

    if (player_) {
        player_->Draw();
    }

    if (boss_) {
        boss_->Draw();
    }
}

void GameScene::PauseUpdate() {}
void GameScene::PauseDraw() {}