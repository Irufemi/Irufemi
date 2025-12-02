#include "GameScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"


#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include"contents/GameFunction.h"

// デストラクタ
GameScene::~GameScene() {

}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {

    // 参照したものをコピー
    // エンジン
    this->engine_ = engine;

    camera_ = std::make_unique <Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 5.0f, -10.0f });
    camera_->SetRotate(Vector3{ -6.0f, 0.0f, 0.0f });

    debugCamera_ = std::make_unique <DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode = false;

    pointLight_ = std::make_unique <PointLightClass>();
    pointLight_->Initialize();
    pointLight_->SetPos(Vector3{ 0.0f,-5.0f,0.0f });

    engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

    spotLight_ = std::make_unique <SpotLightClass>();
    spotLight_->Initialize();
    spotLight_->SetIntensity(0.0f);

    engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

    playerObj_ = std::make_unique<SphereClass>();
    playerObj_->Initialize(camera_.get());

    player_ = std::make_unique<Player>();
    player_->Initialize(camera_.get(), playerObj_.get(), Vector3{ 0.0f, 0.0f, 0.0f },
        engine->GetInputManager());

    //岩の初期化
    rockManager_ = std::make_unique<RockManager>();
    rockManager_->Initialize(camera_.get());  // ← camera_ を渡す
    rockManager_->SetSpawnArea(
        Vector3{ -10.0f, 0.0f, 5.0f },
        Vector3{ 10.0f, 0.0f, 15.0f }
    );
}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("GameScene");
    // pointLight 
    pointLight_->Debug();
    // spotLight 
    spotLight_->Debug();

    ImGui::End();

    ImGui::Begin("Texture");
    if (ImGui::Button("allLoadActivate")) {
        engine_->GetTextureManager()->LoadAllFromFolder("resources/");
    }
    ImGui::Checkbox("debugMode", &debugMode);
    ImGui::End();

#endif // _DEBUG

    // カメラの更新
    if (debugMode) {
        debugCamera_->Update();
        camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
        camera_->SetPerspectiveFovMatrix(debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    }
    else {
        camera_->Update("Camera");
    }

    if (engine_->GetInputManager()->IsKeyPressed('P') || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
        engine_->GetSceneManager()->Request("Title");
    }

    // プレイヤーの更新処理
    player_->Update();

    //岩の更新
    if (rockManager_) {
        rockManager_->Update(player_.get());
    }

    //岩とプレイヤーのあたり判定
    if (player_ && rockManager_) {
        GameFunction::CheckHit_PlayerAndRock(
            *player_,
            rockManager_->GetRocks()
        );
    }
}

// 描画
void GameScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // プレイヤーの更新処理
    player_->Draw();

    // Region
    engine_->ApplyRegionPSO();
    // 岩の描画
    if (rockManager_) {
        rockManager_->Draw(engine_, camera_.get());
    }


    // Particle
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyParticlePSO();

    // Sprite

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

}


