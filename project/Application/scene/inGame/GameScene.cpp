#include "GameScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "2D/Sprite.h"

#include "function/Random.h"
#include "function/Collision.h"

// デストラクタ
GameScene::~GameScene() {


    while (!enemies_.empty()) {
        Enemy* e = enemies_.front();
        if (e) delete e;
        enemies_.pop_front();
    }

    while (!walls_.empty()) {
        Wall* w = walls_.front();
        if (w) delete w;
        walls_.pop_front();
    }

    // スマートポインタに変更したため下記はコメントアウト

    /*delete player_;
    delete model_;
    delete healer_;*/
}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {

    // 参照したものをコピー
    // エンジン
    this->engine_ = engine;

    camera_ = std::make_unique <Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique <DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

    // --- ライトの初期化 ---
    pointLight_ = std::make_unique <PointLight>();
    pointLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight_->position = { 0.0f, 5.0f, 0.0f };
    pointLight_->intensity = 1.0f;
    pointLight_->radius = 10.0f;
    pointLight_->decay = 1.0f;

    spotLight_ = std::make_unique <SpotLight>();
    spotLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    spotLight_->position = { 2.0f, 1.25f, 0.0f };
    spotLight_->distance = 7.0f;
    spotLight_->direction = Math::Normalize(Vector3{ -1.0f,-1.0f,0.0f });
    spotLight_->intensity = 0.0f; // 初期状態ではOFF
    spotLight_->decay = 2.0f;
    spotLight_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;


    // ポーズ画面用スプライト
    pauseSprite_ = std::make_unique<Sprite>();
    pauseSprite_->Initialize(camera_.get(), "resources/whiteTexture.png");
    pauseSprite_->SetPosition(engine->GetClientWidth() / 2.0f, engine->GetClientHeight() / 2.0f);
    pauseSprite_->SetSize(static_cast<float>(engine->GetClientWidth()), static_cast<float>(engine->GetClientHeight()));
    pauseSprite_->SetAnchor(0.5f, 0.5f);
    pauseSprite_->SetColor({ 0.1f, 0.1f, 0.1f, 0.5f });

    // ランダムエンジン
    Random::SeedEngine();

#pragma region Player初期化
    player_ = std::make_unique<Player>();
    player_->Initialize(camera_.get(), Vector3{ -5.0f, 0.0f, 0.0f }, engine_->GetInputManager());
#pragma endregion Player初期化

#pragma region Wall初期化

    {
        const float radius = 20.0f;
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        for (int32_t i = 0; i < kMaxWall_; ++i) {
            float angle = twoPi * static_cast<float>(i) / static_cast<float>(kMaxWall_);
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            Wall* wall = new Wall();
            wall->Initialize(camera_.get(), Vector3{x, y, 0.0f});

            float rotZ = angle + std::numbers::pi_v<float> *0.5f;
            wall->SetRotation(Vector3{ 0.0f, 0.0f, rotZ });
            walls_.push_back(wall);
        }
    }

#pragma endregion Wall初期化

#pragma region Enemy初期化
    for (int32_t i = 0; i < kMaxEnemy_; ++i) {
        Enemy* enemy = new Enemy();

        float x = Random::GeneratorFloat(-10.0f, 10.0f);
        float y = Random::GeneratorFloat(-10.0f, 10.0f);
        enemy->Initialize(camera_.get(), Vector3{x, y, 0.0f});
        enemies_.push_back(enemy);
    }
#pragma endregion Enemy初期化 "Cube.obj");

    // Healer 初期化
    healer_ = std::make_unique<Healer>();

}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("GameScene");

    // pointLight 
    if (ImGui::CollapsingHeader("PointLight")) {
        ImGui::ColorEdit4("PointLightColor", &pointLight_->color.x);
        ImGui::DragFloat3("PointLightPosition", &pointLight_->position.x, 0.01f);
        ImGui::DragFloat("PointLightIntensity", &pointLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat("PointLightRadius", &pointLight_->radius, 0.01f, 0.0f);
        ImGui::DragFloat("PointLightDecay", &pointLight_->decay, 0.01f, 0.0f);
    }
    // spotLight 
    if (ImGui::CollapsingHeader("SpotLight")) {
        ImGui::ColorEdit4("SpotLightColor", &spotLight_->color.x);
        ImGui::DragFloat3("SpotLightPosition", &spotLight_->position.x, 0.01f);
        ImGui::DragFloat("SpotLightIntensity", &spotLight_->intensity, 0.01f, 0.0f);
        ImGui::DragFloat3("SpotLightDirection", &spotLight_->direction.x, 0.01f);
        spotLight_->direction = Math::Normalize(spotLight_->direction);
        ImGui::DragFloat("SpotLightDistance", &spotLight_->distance, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightDecay", &spotLight_->decay, 0.01f, 0.0f);
        ImGui::DragFloat("SpotLightCosAngle", &spotLight_->cosAngle, 0.01f, 0.0f, 1.0f);
    }
    // directionalLight
    if (ImGui::CollapsingHeader("DirectionalLight")) {
        ImGui::ColorEdit4("DirectionalLightColor", &directionalLight_->color.x);
        ImGui::DragFloat3("DirectionalLightDirection", &directionalLight_->direction.x, 0.01f);
        directionalLight_->direction = Math::Normalize(directionalLight_->direction);
        ImGui::DragFloat("DirectionalLightIntensity", &directionalLight_->intensity, 0.01f, 0.0f);
    }

    ImGui::End();

    ImGui::Begin("Texture");
    if (ImGui::Button("allLoadActivate")) {
        engine_->GetTextureManager()->LoadAllFromFolder("resources/");
    }
    ImGui::Checkbox("debugMode", &debugMode_);
    ImGui::End();

#endif // _DEBUG

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // =====
    // ↓ゲームの更新
    // =====

    for (int32_t i = 0; i < kMaxWall_; ++i) {
        Wall* w = walls_.front();
        if (w) w->Update();
        walls_.push_back(walls_.front());
        walls_.pop_front();
    }

    for (int32_t i = 0; i < kMaxEnemy_; ++i) {
        Enemy* e = enemies_.front();
        if (e) e->Update(walls_);
        enemies_.push_back(enemies_.front());
        enemies_.pop_front();
    }
    player_->Update();

    CollisionCheck();

    // Healer は壊れた順に修復を試みる
    if (healer_) healer_->Update(camera_.get(), walls_);

    // =====
    // ↑ゲームの更新
    // =====

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();
    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, *pointLight_, *spotLight_);
}

// 描画
void GameScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO(); 
    
    for (int32_t i = 0; i < kMaxWall_; ++i) {
        Wall* w = walls_.front();
        if (w) w->Draw();
        walls_.push_back(walls_.front());
        walls_.pop_front();
    }

    for (int32_t i = 0; i < kMaxEnemy_; ++i) {
        Enemy* e = enemies_.front();
        if (e) e->Draw();
        enemies_.push_back(enemies_.front());
        enemies_.pop_front();
    }

    //player_->Draw();

    // Sprite
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();
}

void GameScene::PauseUpdate()
{
    // ポーズ中の更新処理
    if (pauseSprite_) {
        pauseSprite_->Update();
    }
}

void GameScene::PauseDraw()
{
    // ポーズ画面の描画
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();
}

void GameScene::CollisionCheck() {
#pragma region OBB更新
    if (!player_)
        return;

    // プレイヤーと血管の OBB を更新
    player_->UpdateOBB();
    const OBB& obbPlayer = player_->GetOBB();
#pragma endregion OBB更新

#pragma region PlayerとWallの衝突判定
    for (Wall* wall : walls_) {
        if (!wall)
            continue;
        wall->UpdateOBB();
        const OBB& obbWall = wall->GetOBB();

        if (Collision::IsOBBCollision(obbPlayer, obbWall)) {
            player_->HandleCollision();
        }
    }
#pragma endregion PlayerとWallの衝突判定

#pragma region PlayerとEnemyの衝突判定
    for (Enemy* enemy : enemies_) {
        if (!enemy)
            continue;

        // 敵の OBB を更新して取得
        enemy->UpdateOBB();
        const OBB& obbEnemy = enemy->GetOBB();

        // Player と Enemy の衝突判定
        if (Collision::IsOBBCollision(obbPlayer, obbEnemy)) {
            player_->HandleCollision();
            enemy->HandleCollision();
        }
    }
#pragma endregion PlayerとEnemyの衝突判定

    // Enemy と Wall の衝突判定（接触フレームを蓄積して HP を減らす）
    for (auto wallIt = walls_.begin(); wallIt != walls_.end(); ++wallIt) {
        Wall* wall = *wallIt;
        if (!wall) continue;

        wall->UpdateOBB();
        const OBB& obbWall = wall->GetOBB();

        bool touched = false;

        for (auto enemyIt = enemies_.begin(); enemyIt != enemies_.end(); ++enemyIt) {
            Enemy* enemy = *enemyIt;
            if (!enemy || !enemy->IsAlive()) continue;

            enemy->UpdateOBB();
            const OBB& obbEnemy = enemy->GetOBB();

            if (Collision::IsOBBCollision(obbEnemy, obbWall)) {
                touched = true;
                bool destroyed = wall->AccumulateContactFrame();
                if (destroyed) {

                    for (auto eIt = enemies_.begin(); eIt != enemies_.end(); ++eIt) {
                        Enemy* e = *eIt;
                        if (!e || !e->IsAlive()) continue;
                        e->UpdateOBB();
                        if (Collision::IsOBBCollision(e->GetOBB(), obbWall)) {
                            e->Kill();
                        }
                    }

                    // 壊された位置を Healer に通知
                    if (healer_) healer_->NotifyWallDestroyed(wall->GetPosition(), wall->GetRotation());

                    delete wall;
                    *wallIt = nullptr;
                    break; // この壁は破壊されたので次の壁へ
                }
            }
        }

        if (*wallIt != nullptr && !touched) {
            // 徐々に接触フレームを減らし、断続的な接触でもHPが減るようにする
            wall->DecayContactFrames();
        }
    }
}
