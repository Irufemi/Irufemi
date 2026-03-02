#include "GameScene.h"

// --- 重要：Playerの定義を最初に見せる ---
#include "actors/player/Player.h" // プロジェクトのフォルダ構造に合わせてパスを調整してください

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

// コンストラクタ
GameScene::GameScene() {}

// デストラクタ：ここでPlayerの定義が既にあるため unique_ptr が正常に削除できる
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

    // エラー修正：Enemy::Initialize は Camera* を求めているため、camera_.get() を渡す
    boss_ = std::make_unique<Enemy>();
    boss_->Initialize(camera_.get()); // ここを修正

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLight_->direction = { 0.0f, -1.0f, 1.0f };
    directionalLight_->intensity = 1.0f;
}

void GameScene::Update() {
#ifdef USE_IMGUI
    ImGui::Begin("Debug");
    ImGui::Checkbox("Debug Camera", &debugMode_);
    ImGui::End();
#endif

    Camera* currentCamera = nullptr;
    if (debugMode_) {
        debugCamera_->Update();
        currentCamera = const_cast<Camera*>(&debugCamera_->GetCamera());
    } else {
        if (player_) {
            player_->Update();
        }
        camera_->Update("Main Camera");
        currentCamera = camera_.get();
    }

    if (boss_) {
        boss_->Update();
    }

    // GPUデータ転送
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();

    std::vector<PointLight*> pLights;
    std::vector<SpotLight*> sLights;
    std::vector<AreaLight*> aLights;

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);
}

void GameScene::Draw() {
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);
    engine_->ApplyPSO();

    if (player_) {
        player_->Draw();
    }

    if (boss_) {
        boss_->Draw();
    }
}

void GameScene::PauseUpdate() {}
void GameScene::PauseDraw() {}