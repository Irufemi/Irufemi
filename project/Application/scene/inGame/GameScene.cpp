#define NOMINMAX
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

#include "Sword.h"
#include "actors/enemy/TwoHitEnemy.h"

#include "3D/Effect/EffectSystem.h"

// 
void GameScene::ClearAllObjects() {
    // 1. 生ポインタのリストを delete してクリア
    for (Wall* w : walls_) { if (w) delete w; }
    walls_.clear();

    for (Enemy* e : enemies_) { if (e) delete e; }
    enemies_.clear();

    for (HealerActor* ha : healerActor_) { if (ha) delete ha; }
    healerActor_.clear();

    // 2. unique_ptr をリセット
    player_.reset();
    healer_.reset();
    //timeDisplay_.reset();

    // 3. ライトのリストもリセット（必要に応じて）
    // ※ Initializeで毎回 push_back するなら、ここをクリアしないと増え続けます
    //pointLights_.clear();
    //spotLights_.clear();
    //areaLights_.clear();
}

// デストラクタ
GameScene::~GameScene() {


    ClearAllObjects();
    timeDisplay_.reset();
    pointLights_.clear();
    spotLights_.clear();
    areaLights_.clear();

}

// 初期化
void GameScene::Initialize(IrufemiEngine* engine) {

    // 1. 基本設定 (phase_, mode_, engine_)

#if defined(_DEBUG) || defined(DEVELOPMENT)
    // phaseの設定
    phase_ = Phase::FadeIn;
    // modeの設定(ゲーム自体を見たいため一旦Standard)
    mode_ = GameMode::Standard;
#else
    //phaseの設定
    phase_ = Phase::Game;
    // modeの設定(Releaseは間違えないようにtutorialから)
    mode_ = GameMode::Standard;

#endif

    // Phaseを初期化したかをfalseに
    isResetPhase_ = false;

    // Phaseを完了したかをfalseに
    isCompletePhase_ = false;

    // 参照したものをコピー
    // エンジン
    this->engine_ = engine;

    // 2. システム基盤 (camera_, light_, pauseSprite_)

    camera_ = std::make_unique <Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique <DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

#pragma region takamura_追加
    // エフェクトシステムの初期化
    effectSystem_ = std::make_unique<EffectSystem>();
    effectSystem_->Initialize(camera_.get());
#pragma endregion

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

    // フェード用スプライト
    fadeSprite_ = std::make_unique<Sprite>();
    fadeSprite_->Initialize(camera_.get(), "resources/whiteTexture.png");
    fadeSprite_->SetPosition(engine->GetClientWidth() / 2.0f, engine->GetClientHeight() / 2.0f);
    fadeSprite_->SetSize(static_cast<float>(engine->GetClientWidth()), static_cast<float>(engine->GetClientHeight()));
    fadeSprite_->SetAnchor(0.5f, 0.5f);
    fadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f }); // 初期は黒

    // ランダムエンジン
    Random::SeedEngine();

    // 3. モード別の初期化
    ModeInitialize();

    // 4. 全モード共通UIの生成

    // 時間表記の生成・初期化
    timeDisplay_ = std::make_unique<TimeDisplay>();
    timeDisplay_->Initialize(
        camera_.get(),
        TimeFormat::S_DECIMAL,
        "resources/texture/text_num.png", { 32.0f, 64.0f },
        "resources/texture/timeDisplay_separator.png", { 32.0f, 64.0f }
    );
    timeDisplay_->SetPosition({ 20.0f, 20.0f }); // 左上に配置


    // 5. フェーズ初期化
    PhaseInitialize();

    isTransitioningToStandard_ = false;

	sePlayerHit_.Initialize("resources/audio/se/PlayerHit.mp3");

}

// 更新
void GameScene::Update() {

#if defined USE_IMGUI
    DebugImGui();
#endif // USE_IMGUI

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // 

    // =====
    // ↓ゲームの更新
    // =====

    PhaseUpdate();

    // フェード中以外にゲームオブジェクトを更新
    if (phase_ != Phase::FadeIn && phase_ != Phase::FadeOut) {
        // Update all walls (use full container size because we now create multiple rings)
        for (Wall* w : walls_) {
            if (w) w->Update();
        }

        // --- Enemy の更新 ---
        for (Enemy* e : enemies_) {
            if (e) e->Update(walls_, healerActor_);
        }

        // --- HealerActor の更新 ---
        for (HealerActor* ha : healerActor_) {
            if (ha) ha->Update();
        }

        player_->Update();

        CollisionCheck();

        // Healer は壊れた順に修復を試みる
        if (healer_) healer_->Update(camera_.get(), walls_, healerActor_);

        // エフェクトの更新
        if (effectSystem_) {
            effectSystem_->Update();
        }
    }


    // 時間表示の更新
    if (timeDisplay_) {
        timeDisplay_->Update();
    }

     if (cameraShakeTimer_ > 0.0f && camera_) {
        cameraShakeTimer_ -= 1.0f / 60.0f;
       
        float mag = cameraShakeMagnitude_;
        float ox = (Random::GeneratorFloat(-1.0f, 1.0f)) * mag;
        float oy = (Random::GeneratorFloat(-1.0f, 1.0f)) * mag;
        Vector3 t = camera_->GetTranslate();
      
        if (cameraShakeTimer_ + (1.0f/60.0f) >= cameraShakeDuration_) {
            cameraShakeOriginalTranslate_ = t;
        }
        camera_->SetTranslate(Vector3{ cameraShakeOriginalTranslate_.x + ox, cameraShakeOriginalTranslate_.y + oy, cameraShakeOriginalTranslate_.z });
        if (cameraShakeTimer_ <= 0.0f) {
          
            camera_->SetTranslate(cameraShakeOriginalTranslate_);
            cameraShakeTimer_ = 0.0f;
        }
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

// 描画
void GameScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // Draw all walls (iterate whole container to include added rings)
    for (Wall* w : walls_) {
        if (w) w->Draw();
    }

    for (Enemy* e : enemies_) {
        if (e) e->Draw();
    }

    for (HealerActor* ha : healerActor_) {
        if (ha) ha->Draw();
    }

    player_->Draw();

    // エフェクトの描画
    if (effectSystem_) {
        effectSystem_->Draw();
    }

    // Sprite
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // カウントダウンタイマーの描画
    if (timeDisplay_) {
        float remainingTime = playTime_ - timer_;
        if (remainingTime < 0.0f) {
            remainingTime = 0.0f;
        }
        timeDisplay_->Draw(remainingTime);
    }

    // フェードスプライトの描画
    if (fadeSprite_ && (phase_ == Phase::FadeIn || phase_ == Phase::FadeOut)) {
        fadeSprite_->Draw();
    }
}

// フェーズの初期化
void GameScene::PhaseInitialize() {

    // 早期リターン
    if (isResetPhase_) {
        return;
    }

    switch (phase_)
    {
    case Phase::FadeIn:
        FadeInInitialize();
        break;
    case Phase::Countdown:

        break;
    case Phase::Game:

        // ゲームタイマーの初期化
        timer_ = 0.0f;
        isGameOver_ = false;

        break;
    case Phase::FadeOut:
        FadeOutInitialize();
    default:
        break;
    }
    // 初期化の完了
    isResetPhase_ = true;
}

// フェーズの更新
void GameScene::PhaseUpdate() {
    // 完了していたら次のフェーズへ
    if (isCompletePhase_) {
        PhaseChange();
        isCompletePhase_ = false; // フラグをリセット
        isResetPhase_ = false;    // 初期化フラグをリセット
    }

    // フェーズ初期化
    PhaseInitialize();

    // 各フェーズの更新
    switch (phase_) {
    case Phase::FadeIn:
        FadeInUpdate();
        break;
    case Phase::Countdown:
        // CountdownUpdate(); // (必要なら)
        break;
    case Phase::Game:
        GameUpdate();
        break;
    case Phase::FadeOut:
        FadeOutUpdate();
        break;
    }
}

// フェーズの変更
void GameScene::PhaseChange() {

    switch (phase_)
    {
    case Phase::FadeIn:
        phase_ = Phase::Countdown;
        break;
    case Phase::Countdown:
        phase_ = Phase::Game;
        break;
    case Phase::Game:
        phase_ = Phase::FadeOut;
        // チュートリアル完了後なら移行フラグを立てる
        if (mode_ == GameMode::Tutorial) {
            isTransitioningToStandard_ = true;
        }
        break;
    case Phase::FadeOut:
        // チュートリアルからの移行ならFadeInに戻る
        if (isTransitioningToStandard_) {
            mode_ = GameMode::Standard;
            ModeInitialize();
            phase_ = Phase::FadeIn;
            isTransitioningToStandard_ = false; // フラグをリセット
        }
        else {
            // スタンダードモード終了後はタイトルなどへ
            engine_->GetSceneManager()->Request("Title");
        }
    default:
        break;
    }

    isResetPhase_ = false;
}

// フェードインの初期化
void GameScene::FadeInInitialize() {
    fadeTimer_ = 0.0f;
}

// フェードイン中の更新
void GameScene::FadeInUpdate() {

    fadeTimer_ += 1.0f / 60.0f;
    float alpha = 1.0f - std::min(fadeTimer_ / kFadeDuration_, 1.0f);
    fadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });
    fadeSprite_->Update();

    if (fadeTimer_ >= kFadeDuration_) {
        isCompletePhase_ = true;
    }

}

// ゲーム中の更新
void GameScene::GameInitialize() {
    switch (mode_) {
    case GameMode::Tutorial:

        break;
    case GameMode::Standard:

    default:
        break;
    }
}


 // ゲーム中の更新
void GameScene::GameUpdate() {
    switch (mode_) {
    case GameMode::Tutorial:

        // チュートリアル完了条件（例：Healerがいなくなったら）
        if (!healer_) {
            isCompletePhase_ = true;
        }

        break;
    case GameMode::Standard:
    default:

        // タイマー更新 (60FPS固定と仮定)
        timer_ += 1.0f / 60.0f;

        // クリア条件：60秒経過し、かつゲームオーバーでない
        if (timer_ >= playTime_ && !isGameOver_) {
            isCompletePhase_ = true;
        }

        // ゲームオーバー条件：2層目の壁が破壊された
        if (isGameOver_) {
            isCompletePhase_ = true;
        }

        break;
    }
}

// フェードアウト中の更新
void GameScene::FadeOutInitialize() {
    fadeTimer_ = 0.0f;
}


// フェードアウト中の更新
void GameScene::FadeOutUpdate() {

    fadeTimer_ += 1.0f / 60.0f;
    float alpha = std::min(fadeTimer_ / kFadeDuration_, 1.0f);
    fadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });
    fadeSprite_->Update();

    if (fadeTimer_ >= kFadeDuration_) {
        isCompletePhase_ = true;
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


    if (pauseSprite_) {
        pauseSprite_->Draw();
    }
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
            // 衝突したらプレイヤーの位置が戻るので、OBBを再更新してループを抜ける
            player_->UpdateOBB();
            break;
        }
    }
#pragma endregion PlayerとWallの衝突判定

#pragma region PlayerとEnemyの衝突判定
    for (Enemy* enemy : enemies_) {
        if (!enemy || !enemy->IsAlive())
            continue;

        // 敵の OBB を更新して取得
        enemy->UpdateOBB();
        const OBB& obbEnemy = enemy->GetOBB();

        // Player と Enemy の衝突判定
        if (Collision::IsOBBCollision(obbPlayer, obbEnemy)) {
            player_->HandleCollision();
            enemy->HandleCollision();
           
            if (cameraShakeTimer_ <= 0.0f) {
                const float shakeDur = 0.8f;
                StartCameraShake(camera_.get(), shakeDur, 0.8f);
                
                player_->StunFor(shakeDur);
            }
            // SE 再生: プレイヤーと敵が接触したとき
            sePlayerHit_.Play(false);
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

                    // ゲームオーバー判定
                    if (wall->GetRingIndex() == 1) { // 2層目 (0-indexed)
                        isGameOver_ = true;
                    }

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


            for (auto enemyIt = enemies_.begin(); enemyIt != enemies_.end(); ++enemyIt) {
                Enemy* enemy = *enemyIt;
                if (!enemy || !enemy->IsAlive()) continue;
                enemy->UpdateOBB();
                const OBB& enemyObb = enemy->GetOBB();
                if (Collision::IsOBBCollision(swordObb, enemyObb)) {

                    // ヒットエフェクトを敵の位置で発生
                    if (effectSystem_) {
                        Transform hitTransform;
                        hitTransform.translate = enemyObb.center;  // 敵の中心位置
                        effectSystem_->Play(EffectType::kHitEffect, hitTransform);
                    }

                    //enemy->Kill();

                   // call virtual HitBySword so derived enemies can require multiple hits
                    enemy->HitBySword();

                }
            }
        }
    }
#pragma endregion SwordとEnemyの衝突判定
}

void GameScene::StandardInitialize() {
    // --- 共通UI（ここでもリセットされるので再生成） ---
    // timeDisplay_ = std::make_unique<TimeDisplay>();
    // timeDisplay_->Initialize(camera_.get(), TimeFormat::S_DECIMAL, "resources/texture/text_num.png", {32, 64}, "...", {32, 64});

    // --- Player ---
    player_ = std::make_unique<Player>();
    player_->Initialize(camera_.get(), Vector3{ -5.0f, 0.0f, 0.0f }, engine_->GetInputManager());

    // 斬撃時のエフェクトコールバックを設定
   /* if (player_->GetSword()) {
        player_->GetSword()->SetOnSlashStart([this](const Transform& t) {
            if (effectSystem_) {
                effectSystem_->Play(EffectType::kHitEffect, t);
            }
        });
    }*/

    // --- Walls (二重リング配置) ---
    {
        Wall sampleWall; // サイズ取得用のサンプル
        const float wallHeight = sampleWall.GetHeight();
        const float baseRadius = 20.0f;
        const float radii[2] = { baseRadius, baseRadius + wallHeight };
        const float twoPi = 2.0f * std::numbers::pi_v<float>;

        // Create two concentric rings: inner and one outer ring.
        for (int ring = 0; ring < 2; ++ring) {
            float radius = radii[ring];
            // Stagger every other ring so walls are not perfectly aligned radially
            float angularOffset = 0.0f;

            for (int32_t i = 0; i < kMaxWall_; ++i) {
                float angle = twoPi * static_cast<float>(i) / static_cast<float>(kMaxWall_) + angularOffset;
                float x = radius * std::cos(angle);
                float y = radius * std::sin(angle);
                Wall* wall = new Wall();
                wall->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
                wall->SetRingIndex(ring); // リングのインデックスを設定

                if (ring > 0) {
                    float scaleRatio = radii[ring] / radii[0];
                    wall->SetScale({ scaleRatio, 1.0f, 1.0f });
                }

                float rotZ = angle + std::numbers::pi_v<float> *0.5f;
                wall->SetRotation(Vector3{ 0.0f, 0.0f, rotZ });
                walls_.push_back(wall);
            }
        }
    }

    // --- Enemies ---
    for (int32_t i = 0; i < kMaxEnemy_; ++i) {
        // Create one TwoHitEnemy (requires 2 sword hits) for the first slot, others are normal Enemies
        Enemy* enemy = nullptr;
        if (i == 0) {
            auto* e2 = new TwoHitEnemy();
            enemy = e2;
        } else {
            enemy = new Enemy();
        }

        float x = Random::GeneratorFloat(-10.0f, 10.0f);
        float y = Random::GeneratorFloat(-10.0f, 10.0f);
        enemy->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
        enemies_.push_back(enemy);
    }

    healer_ = std::make_unique<Healer>();

    //Healer が壁破壊時に必要に応じてスポーン。
}

void GameScene::TutorialInitialize() {
    // チュートリアル用の簡易的な配置
    player_ = std::make_unique<Player>();
    player_->Initialize(camera_.get(), Vector3{ 0, 0, 0 }, engine_->GetInputManager());

    //// コールバック設定
    //if (player_->GetSword()) {
    //    player_->GetSword()->SetOnSlashStart([this](const Transform& t) {
    //        if (effectSystem_) {
    //            effectSystem_->Play(EffectType::kHitEffect, t);
    //        }
    //     });
    //}

    // 壁を1枚だけ置くなど
    Wall* wall = new Wall();
    wall->Initialize(camera_.get(), Vector3{ 0, 5, 0 });
    walls_.push_back(wall);

    healer_ = std::make_unique<Healer>();
}

void GameScene::ModeInitialize() {
    // 既存のオブジェクト（Player, Enemy, Wall等）をすべて消す
    ClearAllObjects();

    // ゲームオーバーフラグをリセット
    isGameOver_ = false;

    // モードに応じて生成
    switch (mode_) {
    case GameMode::Tutorial:
        TutorialInitialize();
        break;
    case GameMode::Standard:
        StandardInitialize();
        break;
    }
}
void GameScene::StartCameraShake(Camera* cam, float duration, float magnitude) {
    if (!cam) return;
    cameraShakeDuration_ = duration;
    cameraShakeTimer_ = duration;
    cameraShakeMagnitude_ = magnitude;
    cameraShakeOriginalTranslate_ = cam->GetTranslate();
}

#if defined(USE_IMGUI)
void GameScene::DebugImGui()
{
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
            ImGui::Text("GameOver: %s", isGameOver_ ? "true" : "false");

            // GameModeの変更
            const char* modeNames[] = { "Tutorial", "Standard" };
            int currentMode = static_cast<int>(mode_);
            if (ImGui::Combo("GameMode", &currentMode, modeNames, IM_ARRAYSIZE(modeNames))) {
                mode_ = static_cast<GameMode>(currentMode);
                ModeInitialize(); // モードを再初期化
            }

            // Phaseの変更
            const char* phaseNames[] = { "FadeIn", "Countdown", "Game", "FadeOut" };
            int currentPhase = static_cast<int>(phase_);
            if (ImGui::Combo("Phase", &currentPhase, phaseNames, IM_ARRAYSIZE(phaseNames))) {
                phase_ = static_cast<Phase>(currentPhase);
                isResetPhase_ = false; // フェーズを再初期化
            }

            // チュートリアル強制完了ボタン
            if (mode_ == GameMode::Tutorial) {
                if (ImGui::Button("Force Complete Tutorial")) {
                    isCompletePhase_ = true;
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}
#endif