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
#include "math/AreaLight.h"
#include "2D/Sprite.h"
#include "contents/UI/NumberText.h"

#include "function/Random.h"
#include "function/Collision.h"

#include "../../Sword.h" // add include for Sword

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

    while (!healerActor_.empty()) {
        HealerActor* ha = healerActor_.front();
        if (ha)
            delete ha;
        healerActor_.pop_front();
    }

    // スマートポインタに変更したため下記はコメントアウト

    /*delete player_;
    delete model_;
    delete healer_;*/
}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {

    // Phaseを初期化したかをfalseに
    isResetPhase_ = false;

    // Phaseを完了したかをfalseに
    isCompletePhase_ = false;

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
    // シーンに最初から配置するライト
    auto pointLight = std::make_unique<PointLight>();
    pointLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight->position = { 0.0f, 5.0f, 0.0f };
    pointLight->intensity = 1.0f;
    pointLight->radius = 10.0f;
    pointLight->decay = 1.0f;
    pointLight->isActive = 1;
    pointLights_.push_back(std::move(pointLight));

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

    // フェーズに応じた初期化を行う
    PhaseInitialize();

    // タイマーの初期化
    timer_ = 0.0f;

    // 時間表記の生成・初期化
    timeDisplay_ = std::make_unique<TimeDisplay>();
    timeDisplay_->Initialize(
        camera_.get(),
        TimeFormat::S_DECIMAL,
        "resources/texture/text_num.png", { 32.0f, 64.0f },
        "resources/texture/timeDisplay_separator.png", { 32.0f, 64.0f }
    );
    timeDisplay_->SetPosition({ 20.0f, 20.0f }); // 左上に配置

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
#pragma endregion Enemy初期化


#pragma region HealerActor初期化

    for (int32_t i = 0; i < kMaxHealerActor_; ++i) {
        HealerActor* healerActor = new HealerActor();
        float x = Random::GeneratorFloat(-15.0f, 15.0f);
        float y = Random::GeneratorFloat(-15.0f, 15.0f);
        healerActor->Initialize(camera_.get(), Vector3{x, y, 0.0f});
        healerActor_.push_back(healerActor);
    }

#pragma endregion HealerActor初期化


    // Healer 初期化
    healer_ = std::make_unique<Healer>();

}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI

    ImGui::Begin("GameScene");
    if (ImGui::BeginTabBar("GameSceneTabs")) {

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
            ImGui::Text("Timer: %.2f", timer_);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();

#endif // _DEBUG

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // =====
    // ↓ゲームの更新
    // =====

    // タイマー更新 (60FPS固定と仮定)
    timer_ += 1.0f / 60.0f;

    PhaseUpdate();

    for (int32_t i = 0; i < kMaxWall_; ++i) {
        Wall* w = walls_.front();
        if (w) w->Update();
        walls_.push_back(walls_.front());
        walls_.pop_front();
    }

    for (int32_t i = 0; i < kMaxEnemy_; ++i) {
        Enemy* e = enemies_.front();
        if (e) e->Update(walls_, healerActor_);
        enemies_.push_back(enemies_.front());
        enemies_.pop_front();
    }

    for (int32_t i = 0; i < kMaxHealerActor_; ++i)
    {
        HealerActor* ha = healerActor_.front();
        if (ha)
            ha->Update();
        healerActor_.push_back(healerActor_.front());
        healerActor_.pop_front();
    }

    player_->Update();

    CollisionCheck();

    // Healer は壊れた順に修復を試みる
    if (healer_) healer_->Update(camera_.get(), walls_, healerActor_);

    // 時間表示の更新
    if (timeDisplay_) {
        timeDisplay_->Update();
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

// フェーズの初期化
void GameScene::PhaseInitialize() {}

// フェーズの更新
void GameScene::PhaseUpdate() {}

// フェードインの初期化
void GameScene::FadeInInitialize() {}


// フェードイン中の更新
void GameScene::FadeInUpdate() {}


// ゲーム中の更新
void GameScene::GameInitialize() {}


// ゲーム中の更新
void GameScene::GameUpdate() {}

// フェードアウト中の更新
void GameScene::FadeOutInitialize() {}


// フェードアウト中の更新
void GameScene::FadeOutUpdate(){}

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

    for (int32_t i = 0; i < kMaxHealerActor_; ++i)
    {
        HealerActor* ha = healerActor_.front();
        if (ha)
            ha->Draw();
        healerActor_.push_back(healerActor_.front());
        healerActor_.pop_front();
    }

    player_->Draw();

    // Sprite
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // カウントダウンタイマーの描画
    if (timeDisplay_) {
        // 念のため、描画直前にスプライト用の設定を再適用
        engine_->SetBlend(BlendMode::kBlendModeNormal);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine_->ApplySpritePSO();
        float remainingTime = playTime_ - timer_;
        if (remainingTime < 0.0f) {
            remainingTime = 0.0f;
        }
        timeDisplay_->Draw(remainingTime);
    }
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

#pragma region EnemyとWallの衝突判定
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
                enemy->OnCollisionWithWall(wall); // ★ 衝突時に押し出し処理を呼ぶ
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

                    // 壊されたTransformとサイズを Healer に通知
                    if (healer_) healer_->NotifyWallDestroyed(wall->GetTransform(), wall->GetSize());

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

#pragma endregion EnemyとWallの衝突判定

#pragma region EnemyをHealerActorの衝突判定

    // Enemy と HealerActor の衝突判定
    for (auto enemyIt = enemies_.begin(); enemyIt != enemies_.end(); ++enemyIt) {
        Enemy* enemy = *enemyIt;
        if (!enemy || !enemy->IsAlive()) continue;

        enemy->UpdateOBB();
        const OBB& obbEnemy = enemy->GetOBB();

        for (auto healerIt = healerActor_.begin(); healerIt != healerActor_.end(); ++healerIt) {
            HealerActor* ha = *healerIt;
            if (!ha || !ha->IsAlive()) continue;

            ha->UpdateOBB();
            const OBB& obbHealer = ha->GetOBB();

            if (Collision::IsOBBCollision(obbEnemy, obbHealer)) {
                // 双方に衝突ハンドラを呼ぶ
                enemy->HandleCollision();
                ha->HandleCollision();
            }
        }
    }

#pragma endregion EnemyをHealerActorの衝突判定

#pragma region SwordとEnemyの衝突判定
    // Sword の当たり判定はスラッシュ中のみ有効
    if (player_) {
        Sword* sword = player_->GetSword();
        if (sword && sword->IsSlashing()) {
          
            sword->UpdateOBB();
            const OBB& swordObb = sword->GetOBB();

            for (Enemy* enemy : enemies_) {
                if (!enemy || !enemy->IsAlive()) continue;
                enemy->UpdateOBB();
                const OBB& enemyObb = enemy->GetOBB();
                if (Collision::IsOBBCollision(swordObb, enemyObb)) {
                    enemy->Kill();
                }
            }
        }
    }
#pragma endregion SwordとEnemyの衝突判定
}
