#include <cmath>
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
#include "3D/particle/ParticleSystem.h"

#include "function/Random.h"
#include "function/Collision.h"
#include "function/Math.h"

#include "Sword.h"
#include "actors/enemy/TwoHitEnemy.h"
#include "actors/enemy/ChaserEnemy.h"

#include <unordered_map>
#include "scene/GameResultData.h"
#include <unordered_set>



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
    bloodFlowParticleRing_.reset();
    bloodBurstParticle_.reset();
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
    phase_ = Phase::FadeIn;
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

    // ホワイトアウト用スプライト
    whiteoutSprite_ = std::make_unique<Sprite>();
    whiteoutSprite_->Initialize(camera_.get(), "resources/whiteTexture.png");
    whiteoutSprite_->SetPosition(engine->GetClientWidth() / 2.0f, engine->GetClientHeight() / 2.0f);
    whiteoutSprite_->SetSize(static_cast<float>(engine->GetClientWidth()), static_cast<float>(engine->GetClientHeight()));
    whiteoutSprite_->SetAnchor(0.5f, 0.5f);
    whiteoutSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f }); // 最初は透明
    whiteoutSprite_->Update();

    // ランダムエンジン
    Random::SeedEngine();

    // 3. モード別の初期化
    ModeInitialize();

    // 4. 全モード共通UIの生成

    // カウントダウン用スプライトの初期化
    // テクスチャパスは仮のものです。実際のファイル名に合わせてください。
    const char* texturePaths[] = {
        "resources/texture/countdown/start.png", // 0: Start
        "resources/texture/countdown/1.png",     // 1: 1
        "resources/texture/countdown/2.png",     // 2: 2
        "resources/texture/countdown/3.png"      // 3: 3
    };

    for (int i = 0; i < 4; ++i) {
        countdownSprites_[i] = std::make_unique<Sprite>();
        countdownSprites_[i]->Initialize(camera_.get(), texturePaths[i]);
        countdownSprites_[i]->SetAnchor(0.5f, 0.5f);
        countdownSprites_[i]->SetPosition(engine_->GetClientWidth() / 2.0f, engine_->GetClientHeight() / 2.0f);
        countdownSprites_[i]->Update();
    }

    // 説明表示用スプライトの初期化
    instructionSprite_ = std::make_unique<Sprite>();
    instructionSprite_->Initialize(camera_.get(), "resources/texture/game/GameGoal_GameScene.png"); // 仮のパス
    instructionSprite_->SetAnchor(0.5f, 0.5f);
    instructionSprite_->SetPosition(engine_->GetClientWidth() / 2.0f,engine_->GetClientHeight() / 3.0f);
    instructionSprite_->Update();

    // 時間表記の生成・初期化
    timeDisplay_ = std::make_unique<TimeDisplay>();
    timeDisplay_->Initialize(
        camera_.get(),
        TimeFormat::S_DECIMAL,
        "resources/texture/timeDisplay/text_num_", ".png", // ベースパスと拡張子に分割
        { 48.0f, 64.0f },
        "resources/texture/timeDisplay/timeDisplay_separator_dot.png", // '.' 専用テクスチャ
        { 32.0f, 64.0f }
    );
    timeDisplay_->SetPosition({ 20.0f, 20.0f }); // 左上に配置

#pragma region takamura追加（トランジション）
    stripeTransition_ = std::make_unique<StripeTransition>();
#pragma endregion takamura追加


    // 5. フェーズ初期化
    PhaseInitialize();

    isTransitioningToStandard_ = false;

    sePlayerHit_.Initialize("resources/audio/se/PlayerHit.mp3");

    // ヒーラー死亡時のSE初期化
    seHealerDeath_.Initialize("resources/audio/se/DeathHealerActor.mp3");

	bgmGame_.Initialize("resources/audio/bgm/GameScene.mp3", "", true, true);
	bgmGame_.SetVolume(0.8f);

    model_tube_ = std::make_unique<ObjClass>();
    model_tube_->Initialize(camera_.get(), "tube.obj");

    // 血流パーティクルの初期化
    bloodFlowParticle_ = std::make_unique<ParticleSystem>();
    bloodFlowParticle_->Initialize(camera_.get(), "resources/circle.png", ParticleType::kBloodFlow, ParticlePrimitiveShape::Sphere);

    bloodFlowParticleRing_ = std::make_unique<ParticleSystem>();
    bloodFlowParticleRing_->Initialize(camera_.get(), "resources/gradationLine.png", ParticleType::kBloodFlow, ParticlePrimitiveShape::Ring);

    // 血液噴出パーティクルの初期化
    bloodBurstParticle_ = std::make_unique<ParticleSystem>();
    bloodBurstParticle_->Initialize(camera_.get(), "resources/circle.png", ParticleType::kBloodBurst, ParticlePrimitiveShape::Sphere);

    // 画面外インジケーターの初期化
    offScreenIndicator_ = std::make_unique<OffScreenIndicator>();
    offScreenIndicator_->Initialize(camera_.get(), engine_);

    // ポーズ案内用スプライトの初期化
    text_pause_ = std::make_unique<Sprite>();
    text_pause_->Initialize(camera_.get(), "resources/texture/game/pause.png");
    text_pause_->SetAnchor(0.0f, 1.0f);
    text_pause_->SetPosition(0.0f, engine_->GetClientHeight());
    text_pause_->Update();

    // ゲーム中の操作説明用スプライトの初期化
    gameInstructionSprite_ = std::make_unique<Sprite>();
    gameInstructionSprite_->Initialize(camera_.get(), "resources/texture/game/GameGoal_GameScene1.png");
    gameInstructionSprite_->SetAnchor(1.0f, 0.0f);
    gameInstructionSprite_->SetSize(400, 40);
    gameInstructionSprite_->SetPosition(engine_->GetClientWidth(), 0.0f);
    gameInstructionSprite_->Update();
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

    // ゲーム中の更新
    if (phase_ == Phase::Game && gameOverState_ == GameOverState::None) {
        // Update all walls (use full container size because we now create multiple rings)
        for (Wall* w : walls_) {
            if (w) w->Update();
        }

        // --- Enemy の更新 ---
        for (Enemy* e : enemies_) {
            if (e) e->Update(walls_, healerActor_);
        }


        for (auto it = enemies_.begin(); it != enemies_.end(); ++it) {
            Enemy* e = *it;
            if (!e) continue;
            if (e->IsAlive()) continue;
            if (e->GetRespawnCounter() > 0) continue;


            const float minSpawnDist = 3.0f;
            float x = 0.0f, y = 0.0f;
            for (int attempt = 0; attempt < 100; ++attempt) {
                x = Random::GeneratorFloat(-10.0f, 10.0f);
                y = Random::GeneratorFloat(-10.0f, 10.0f);
                Vector3 p = player_ ? player_->GetPosition() : Vector3{ 0,0,0 };
                float dx = x - p.x;
                float dy = y - p.y;
                if ((dx * dx + dy * dy) >= (minSpawnDist * minSpawnDist)) break;
            }


            TwoHitEnemy* two = dynamic_cast<TwoHitEnemy*>(e);
            ChaserEnemy* ch = dynamic_cast<ChaserEnemy*>(e);


            delete e;

            Enemy* spawned = nullptr;
            if (two) {
                auto* nn = new TwoHitEnemy();
                nn->SetModelFile("TD_HardEnemy.obj");
                nn->SetEffectSystem(effectSystem_.get());
                nn->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
                nn->SetPlayer(player_.get());
                spawned = nn;
            } else if (ch) {
                auto* nn = new ChaserEnemy();
                nn->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
                nn->SetPlayer(player_.get());
                spawned = nn;
            } else {
                auto* nn = new Enemy();
                nn->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
                nn->SetPlayer(player_.get());
                spawned = nn;
            }

            *it = spawned;
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

        // 追加: 壁修復後にプレイヤーが壁に埋まっていたら原点方向へ押し出す
        if (player_) {
            player_->UpdateOBB();
            const OBB& obbPlayerFix = player_->GetOBB();
            const Vector3 origin{0.0f, 0.0f, 0.0f};
            // 現在の外周リング半径を計算
            float outerRadius = 0.0f;
            for (Wall* w : walls_) {
                if (!w) continue;
                const Vector3 wp = w->GetPosition();
                outerRadius = std::max(outerRadius, std::sqrt(wp.x * wp.x + wp.y * wp.y));
            }
            const float clampMargin = 0.1f;
            for (Wall* w : walls_) {
                if (!w) continue;
                w->UpdateOBB();
                const OBB& obbWallFix = w->GetOBB();
                if (Collision::IsOBBCollision(obbPlayerFix, obbWallFix)) {
                    // 原点方向への押し出しベクトル
                    Vector3 dir = origin - player_->GetPosition();
                    float len = Math::Length(dir);
                    if (len < 1e-4f) { dir = {1.0f, 0.0f, 0.0f}; len = 1.0f; }
                    dir = dir / len;
                    // 重なり量の概算
                    float rP = std::max({ obbPlayerFix.size.x, obbPlayerFix.size.y, obbPlayerFix.size.z });
                    float rW = std::max({ obbWallFix.size.x, obbWallFix.size.y, obbWallFix.size.z });
                    float dist = Math::Length(obbPlayerFix.center - obbWallFix.center);
                    float overlap = (rP + rW) - dist;
                    float push = std::max(overlap + 0.05f, 0.25f);
                    player_->MoveBy(dir * push);
                    // 外周より外に出ないように半径をクランプ
                    Vector3 p = player_->GetPosition();
                    float r = std::sqrt(p.x * p.x + p.y * p.y);
                    if (outerRadius > 0.0f && r > (outerRadius - clampMargin)) {
                        Vector3 nrm;
                        if (r < 1e-4f) { nrm = {1.0f, 0.0f, 0.0f}; }
                        else { nrm = { p.x / r, p.y / r, 0.0f }; }
                        Vector3 clamped = { nrm.x * (outerRadius - clampMargin), nrm.y * (outerRadius - clampMargin), p.z };
                        player_->MoveBy(clamped - p);
                    }
                    player_->UpdateOBB();
                    break; // 一つ修正したら終了
                }
            }
        }
    }

    // 血流パーティクルの更新
    if (bloodFlowParticle_) {
        bloodFlowParticle_->Update();
#if defined(USE_IMGUI)
        bloodFlowParticle_->Debug("BloodFlow");
#endif
    }
    if (bloodFlowParticleRing_) {
        bloodFlowParticleRing_->Update();
#if defined(USE_IMGUI)
        bloodFlowParticleRing_->Debug("BloodFlowRing");
#endif
    }
    if (bloodBurstParticle_) {
        bloodBurstParticle_->Update();
#if defined(USE_IMGUI)
        bloodBurstParticle_->Debug("BloodBurst");
#endif
    }

    // カメラをプレイヤーに追従させる（デバッグカメラ使用中は追従しない）
    if (!debugMode_ && player_ && camera_) {
        Vector3 playerPos = player_->GetPosition();
        Vector3 camT = camera_->GetTranslate();
        camT.x = playerPos.x;
        camT.y = playerPos.y;
        camera_->SetTranslate(camT);

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

        if (cameraShakeTimer_ + (1.0f / 60.0f) >= cameraShakeDuration_) {
            cameraShakeOriginalTranslate_ = t;
        }
        camera_->SetTranslate(Vector3{ cameraShakeOriginalTranslate_.x + ox, cameraShakeOriginalTranslate_.y + oy, cameraShakeOriginalTranslate_.z });
        if (cameraShakeTimer_ <= 0.0f) {

            camera_->SetTranslate(cameraShakeOriginalTranslate_);
            cameraShakeTimer_ = 0.0f;
        }
    }

#pragma region takamura追加（トランジション）
    if (stripeTransition_) {
        stripeTransition_->Update();
    }
#pragma endregion takamura追加

    // 画面外インジケーターの更新
    if (offScreenIndicator_) {
        offScreenIndicator_->Update(attackingEnemies_);
    }
    model_tube_->Update();

    if (gameInstructionSprite_) {
        gameInstructionSprite_->Update();
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

    model_tube_->Draw();

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

    // 血流パーティクルの描画
    if (bloodFlowParticle_) {
        bloodFlowParticle_->Draw();
    }
    if (bloodFlowParticleRing_) {
        bloodFlowParticleRing_->Draw();
    }
    if (bloodBurstParticle_) {
        bloodBurstParticle_->Draw();
    }

    // Sprite
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // カウントダウンタイマーの描画
    if (phase_ == Phase::Game && timeDisplay_) {
        float remainingTime = playTime_ - timer_;
        if (remainingTime < 0.0f) {
            remainingTime = 0.0f;
        }
        timeDisplay_->Draw(remainingTime);
    }

    // カウントダウンの描画
    if (phase_ == Phase::Countdown) {
        // 説明表示
        if (instructionSprite_) {
            instructionSprite_->Draw();
        }

        // カウントダウン数値表示 (説明時間終了後)
        if (countdownTimer_ <= 3.99f) {
            int spriteIndex = static_cast<int>(std::ceil(countdownTimer_));
            if (spriteIndex >= 0 && spriteIndex < countdownSprites_.size()) {
                if (countdownSprites_[spriteIndex]) {
                    countdownSprites_[spriteIndex]->Draw();
                }
            }
        }
    }

    // 画面外インジケーターの描画
    if (offScreenIndicator_) {
        offScreenIndicator_->Draw();
    }

    // ポーズ案内用スプライトの描画
    text_pause_->Draw();

    if (gameInstructionSprite_) {
        gameInstructionSprite_->Draw();
    }

    // ホワイトアウトの描画
    whiteoutSprite_->Draw();

#pragma region takamura追加（トランジション）
    // 2D描画の最後に
    stripeTransition_->Draw();
#pragma endregion takamura追加
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
        CountdownInitialize();
        break;
    case Phase::Game:

        // ゲームタイマーの初期化
        timer_ = 0.0f;
        isGameOver_ = false;
        gameOverState_ = GameOverState::None;
        gameOverTimer_ = 0.0f;
        if (whiteoutSprite_) {
            whiteoutSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
        }

        break;
    case Phase::FadeOut:
    default:
        FadeOutInitialize();
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
        CountdownUpdate();
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
    default:
        // チュートリアルからの移行ならFadeInに戻る
        if (isTransitioningToStandard_) {
            mode_ = GameMode::Standard;
            ModeInitialize();
            phase_ = Phase::FadeIn;
            isTransitioningToStandard_ = false; // フラグをリセット
        } else {
            // リザルトシーンへ移行する前に結果を保存
            auto& resultData = GameResultData::GetInstance();
            resultData.isGameClear = !isGameOver_;
            resultData.killScore = killScore_;
            engine_->GetSceneManager()->Request("Result");
        }
        break;
    }

    isResetPhase_ = false;
}

// フェードインの初期化
void GameScene::FadeInInitialize() {

    stripeTransition_->Initialize(camera_.get(), engine_, StripeTransition::Mode::Out);
    stripeTransition_->Start();

    // 演出開始前に一度だけ更新処理を呼ぶ
    if (!hasDoneInitialUpdateInFadeIn_) {
        for (Wall* w : walls_) { if (w) w->Update(); }
        for (Enemy* e : enemies_) { if (e) e->Update(walls_, healerActor_); }
        if (player_) player_->Update();
        if (healer_) healer_->Update(camera_.get(), walls_, healerActor_);
        hasDoneInitialUpdateInFadeIn_ = true;
    }
}

// フェードイン中の更新
void GameScene::FadeInUpdate() {

    if (stripeTransition_->IsFinished()) {
        isCompletePhase_ = true;
    }

}

// カウントダウンの初期化
void GameScene::CountdownInitialize() {
    if (mode_ == GameMode::Standard) {
        countdownTimer_ = 3.99f + kInstructionDisplayTime; // 説明時間分タイマーを延長
        hasDoneInitialUpdate_ = false; // 初期化フラグをリセット
    }
}

// カウントダウンの更新
void GameScene::CountdownUpdate() {
    if (mode_ == GameMode::Standard) {
        // 最初の1フレームだけ更新処理を呼ぶ
        if (!hasDoneInitialUpdate_) {
            // FadeInで既に実行済みのため、ここでは不要
            // for (Wall* w : walls_) { if (w) w->Update(); }
            // for (Enemy* e : enemies_) { if (e) e->Update(walls_, healerActor_); }
            // if (player_) player_->Update();
            // if (healer_) healer_->Update(camera_.get(), walls_, healerActor_);
            hasDoneInitialUpdate_ = true;
        }

        countdownTimer_ -= 1.0f / 60.0f; // 60FPS想定

        // 説明表示中はスプライトを更新
        if (instructionSprite_) {
            instructionSprite_->Update();
        }

        // カウントダウン数値のアニメーション処理 (説明時間終了後)
        if (countdownTimer_ <= 3.99f) {
            int spriteIndex = static_cast<int>(std::ceil(countdownTimer_));
            if (spriteIndex >= 0 && spriteIndex < countdownSprites_.size()) {
                Sprite* sprite = countdownSprites_[spriteIndex].get();
                if (sprite) {
                    // 1秒間のアニメーション（拡大してフェードアウト）
                    float progress = 1.0f - (countdownTimer_ - std::floor(countdownTimer_)); // 0.0 -> 1.0
                    float scale = 1.0f + 0.5f * progress;
                    float alpha = 1.0f - progress;

                    Vector2 originalSize = sprite->GetSize();
                    sprite->SetSize(originalSize.x * scale, originalSize.y * scale);
                    Vector4 color = sprite->GetColor();
                    color.w = alpha;
                    sprite->SetColor(color);
                    sprite->Update(); // 変更を反映
                }
            }
        }


        if (countdownTimer_ < -kStartDisplayTime) { // Start表示時間後
            isCompletePhase_ = true;
        }
    } else {
        // チュートリアルモードでは即座にゲームフェーズへ
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

        if (isGameOver_) {
            gameOverTimer_ += 1.0f / 60.0f;

            switch (gameOverState_) {
            case GameOverState::Bursting:
            {
                // 5秒かけてパーティクル表示とホワイトアウトを同時に行う
                const float burstAndFadeDuration = 5.0f;

                // 継続的にパーティクルを放出
                if (bloodBurstParticle_ && (static_cast<int>(gameOverTimer_ * 10) % 5 == 0)) { // 0.5秒ごとに放出
                    bloodBurstParticle_->PlayHitEffect(bloodBurstParticle_->GetEmitterPosition());
                }

                // ホワイトアウトのアルファ値を徐々に上げる
                float alpha = std::min(1.0f, gameOverTimer_ / burstAndFadeDuration);
                if (whiteoutSprite_) {
                    whiteoutSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
                }

                // 演出時間が終了したら完了
                if (gameOverTimer_ >= burstAndFadeDuration) {
                    isCompletePhase_ = true;
                }
            }
            break;
            case GameOverState::FadingOut:
                // この状態は使われなくなるが、念のため残しておく
                isCompletePhase_ = true;
                break;
            case GameOverState::None:
                // 通常ここには来ない
                break;
            }
        } else {
            // タイマー更新 (60FPS固定と仮定)
            timer_ += 1.0f / 60.0f;

            // クリア条件：60秒経過し、かつゲームオーバーでない
            if (timer_ >= playTime_) {
                isCompletePhase_ = true;
            }
        }

        break;
    }
}

// フェードアウト中の更新
void GameScene::FadeOutInitialize() {

    stripeTransition_->Initialize(camera_.get(), engine_, StripeTransition::Mode::In);
    stripeTransition_->Start();
}


// フェードアウト中の更新
void GameScene::FadeOutUpdate() {


    if (stripeTransition_->IsFinished()) {
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

            Vector3 pushDir = obbPlayer.center - obbEnemy.center;
            if (Math::Length(pushDir) < 1e-4f) {

                float ang = Random::GeneratorFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
                pushDir = Vector3{ std::cos(ang), std::sin(ang), 0.0f };
            }
            pushDir = Math::Normalize(pushDir);
            const float knockbackStrength = 3.5f;
            player_->MoveBy(pushDir * knockbackStrength);

            enemy->HandleCollision();

            if (cameraShakeTimer_ <= 0.0f) {
                const float shakeDur = 0.8f;
                StartCameraShake(camera_.get(), shakeDur, 0.8f);

                player_->StunFor(shakeDur);
            }
            // SE 再生: プレイヤーと敵が接触したとき
            sePlayerHit_.Play(false);

            // Enemy dies after hitting the player once
            enemy->Kill();
        }
    }
#pragma region PlayerとEnemyの衝突判定

#pragma region EnemyとWallの衝突判定
    // Enemy と Wall の衝突判定（接触フレームを蓄積して HP を減らす）
    // 1フレームに1体の敵が複数の壁にダメージを与えないように、接触した敵を記録する
    std::unordered_set<Enemy*> contactedEnemies;
    attackingEnemies_.clear(); // 毎フレームクリア
    for (auto wallIt = walls_.begin(); wallIt != walls_.end(); ++wallIt) {
        Wall* wall = *wallIt;
        if (!wall) continue;

        wall->UpdateOBB();
        const OBB& obbWall = wall->GetOBB();

        bool touchedThisWall = false;

        for (auto enemyIt = enemies_.begin(); enemyIt != enemies_.end(); ++enemyIt) {
            Enemy* enemy = *enemyIt;
            if (!enemy || !enemy->IsAlive()) continue;

            // このフレームで既に他の壁に接触済みの敵はスキップ
            if (contactedEnemies.count(enemy)) {
                continue;
            }

            enemy->UpdateOBB();
            const OBB& obbEnemy = enemy->GetOBB();

            if (Collision::IsOBBCollision(obbEnemy, obbWall)) {
                touchedThisWall = true;
                enemy->OnCollisionWithWall(wall); // 衝突時に押し出し処理を呼ぶ
                attackingEnemies_.push_back(enemy); // 攻撃中の敵としてリストに追加

                // この敵を接触済みとして記録
                contactedEnemies.insert(enemy);

                bool destroyed = wall->AccumulateContactFrame();
                if (destroyed) {
                    // ゲームオーバー判定
                    if (wall->GetRingIndex() == 2 && !isGameOver_) { // 3層目 (0-indexed) が破壊されたらゲームオーバー
                        isGameOver_ = true;
                        gameOverState_ = GameOverState::Bursting;
                        gameOverTimer_ = 0.0f;
                        // 血液噴出エフェクトを再生
                        if (bloodBurstParticle_) {
                            bloodBurstParticle_->SetEmitterPosition(wall->GetTransform().translate);
                            bloodBurstParticle_->PlayHitEffect(wall->GetTransform().translate);
                        }
                    }

                    // 壁に接触していた他の敵を処理
                    for (auto eIt = enemies_.begin(); eIt != enemies_.end(); ++eIt) {
                        Enemy* e = *eIt;
                        if (!e || !e->IsAlive()) continue;
                        e->UpdateOBB();
                        if (Collision::IsOBBCollision(e->GetOBB(), obbWall)) {
                            auto* two = dynamic_cast<TwoHitEnemy*>(e);
                            if (two) {
                                two->OnWallDestroyed(wall);
                            } else {
                                e->Kill();
                            }
                        }
                    }

                    // 壊されたTransformとサイズを Healer に通知
                    if (healer_) healer_->NotifyWallDestroyed(wall->GetTransform(), wall->GetSize());

                    delete wall;
                    *wallIt = nullptr;
                    break; // この壁は破壊されたので次の壁へ
                }
            } else {

                float rE = std::max({ obbEnemy.size.x, obbEnemy.size.y, obbEnemy.size.z });
                float rW = std::max({ obbWall.size.x, obbWall.size.y, obbWall.size.z });
                Vector3 d = obbEnemy.center - obbWall.center;
                if (Math::Length(d) <= (rE + rW)) {

                    touchedThisWall = true;
                    enemy->OnCollisionWithWall(wall);
                    attackingEnemies_.push_back(enemy); // 攻撃中の敵としてリストに追加


                    contactedEnemies.insert(enemy);

                    bool destroyed = wall->AccumulateContactFrame();
                    if (destroyed) {
                        if (wall->GetRingIndex() == 2 && !isGameOver_) {
                            isGameOver_ = true;
                            gameOverState_ = GameOverState::Bursting;
                            gameOverTimer_ = 0.0f;
                            // 血液噴出エフェクトを再生
                            if (bloodBurstParticle_) {
                                bloodBurstParticle_->SetEmitterPosition(wall->GetTransform().translate);
                                bloodBurstParticle_->PlayHitEffect(wall->GetTransform().translate);
                            }
                        }
                        for (auto eIt = enemies_.begin(); eIt != enemies_.end(); ++eIt) {
                            Enemy* e = *eIt;
                            if (!e || !e->IsAlive()) continue;
                            e->UpdateOBB();
                            if (Collision::IsOBBCollision(e->GetOBB(), obbWall)) {
                                auto* two = dynamic_cast<class TwoHitEnemy*>(e);
                                if (two) {
                                    two->OnWallDestroyed(wall);
                                } else {
                                    e->Kill();
                                }
                            }
                        }
                        if (healer_) healer_->NotifyWallDestroyed(wall->GetTransform(), wall->GetSize());
                        delete wall;
                        *wallIt = nullptr;
                        break;
                    }
                }
            }
        }

        if (*wallIt != nullptr && !touchedThisWall) {
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
                // ヒーラーが死亡した場合、シーンでSEを再生する
                if (!ha->IsAlive()) {
                    seHealerDeath_.Play(false);
                }
            }
        }
    }

#pragma endregion EnemyをHealerActorの衝突判定

#pragma region Enemy同士の衝突判定
    // Enemy同士がめり込まないように互いに押し出す
    for (auto itA = enemies_.begin(); itA != enemies_.end(); ++itA) {
        Enemy* a = *itA;
        if (!a || !a->IsAlive()) continue;
        a->UpdateOBB();
        const OBB& obbA = a->GetOBB();

        auto itB = itA;
        ++itB;
        for (; itB != enemies_.end(); ++itB) {
            Enemy* b = *itB;
            if (!b || !b->IsAlive()) continue;
            b->UpdateOBB();
            const OBB& obbB = b->GetOBB();

            if (Collision::IsOBBCollision(obbA, obbB)) {
                // 中心間ベクトル
                Vector3 dir = obbA.center - obbB.center;
                float dist = Math::Length(dir);
                float rA = std::max({ obbA.size.x, obbA.size.y, obbA.size.z });
                float rB = std::max({ obbB.size.x, obbB.size.y, obbB.size.z });
                float overlap = rA + rB - dist;
                if (overlap > 0.0001f) {
                    Vector3 pushDir;
                    if (dist < 1e-4f) {
                        // 中心がほぼ一致する場合はランダム方向で押し出す
                        float ang = Random::GeneratorFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
                        pushDir = Vector3{ std::cos(ang), std::sin(ang), 0.0f };
                    } else {
                        pushDir = dir / dist;
                    }
                    Vector3 delta = pushDir * (overlap * 0.5f + 0.001f);
                    a->MoveBy(delta);
                    b->MoveBy(-delta);
                }
            }
        }
    }
#pragma endregion Enemy同士の衝突判定

#pragma region SwordとEnemyの衝突判定
    // Sword の当たり判定はスラッシュ中のみ有効
    if (player_) {
        Sword* sword = player_->GetSword();
        if (sword && sword->IsSlashing()) {

            sword->UpdateOBB();
            const OBB& swordObb = sword->GetOBB();

            // static map to remember which enemy was hit by which slash id
            static std::unordered_map<Enemy*, uint32_t> lastHitSlashIdMap;
            uint32_t currentSlashId = sword->GetCurrentSlashId();

            for (auto enemyIt = enemies_.begin(); enemyIt != enemies_.end(); ++enemyIt) {
                Enemy* enemy = *enemyIt;
                if (!enemy || !enemy->IsAlive()) continue;
                enemy->UpdateOBB();
                const OBB& enemyObb = enemy->GetOBB();
                if (Collision::IsOBBCollision(swordObb, enemyObb)) {

                    // ensure we only register one hit per enemy per slash
                    auto it = lastHitSlashIdMap.find(enemy);
                    if (it != lastHitSlashIdMap.end() && it->second == currentSlashId) {
                        // already hit this enemy during current slash -> skip
                        continue;
                    }
                    // ヒットエフェクトを敵の位置で発生
                    if (effectSystem_) {
                        Transform hitTransform;
                        hitTransform.translate = enemyObb.center;  // 敵の中心位置
                        effectSystem_->Play(EffectType::kHitEffect, hitTransform);
                    }

                    // record this hit
                    lastHitSlashIdMap[enemy] = currentSlashId;

                    // call HitBySlash if enemy is TwoHitEnemy so we pass the slash id and avoid fallback ids
                    TwoHitEnemy* two = dynamic_cast<TwoHitEnemy*>(enemy);
                    if (two) {
                        // debug
                        std::string dbg = "GameScene: registering hit on TwoHitEnemy with slashId=" + std::to_string(currentSlashId) + "\n";
                        OutputDebugStringA(dbg.c_str());
                        two->HitBySlash(currentSlashId);
                    } else {
                        // fallback: call general HitBySword
                        enemy->HitBySword();
                    }

                }
            }

            // Sword と HealerActor の当たり判定（スラッシュ中のみ）
            for (auto haIt = healerActor_.begin(); haIt != healerActor_.end(); ++haIt) {
                HealerActor* ha = *haIt;
                if (!ha || !ha->IsAlive()) continue;

                ha->UpdateOBB();
                const OBB& healerObb = ha->GetOBB();

                if (Collision::IsOBBCollision(swordObb, healerObb)) {
                    // play hit effect at healer position
                    if (effectSystem_) {
                        Transform hitTransform;
                        hitTransform.translate = healerObb.center;
                        effectSystem_->Play(EffectType::kHitEffect, hitTransform);
                    }


                    ha->HandleCollision();


                    if (!ha->IsAlive()) {
                        seHealerDeath_.Play(false);
                    }
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
    player_->Initialize(camera_.get(), Vector3{ 0.0f, 0.0f, 0.0f }, engine_->GetInputManager());

    // 斬撃時のエフェクトコールバックを設定
   /* if (player_->GetSword()) {
        player_->GetSword()->SetOnSlashStart([this](const Transform& t) {
            if (effectSystem_) {
                effectSystem_->Play(EffectType::kHitEffect, t);
            }
        });
    }*/

    // --- Walls (三重リング配置) ---
    {
        Wall sampleWall; // サイズ取得用のサンプル
        const float wallHeight = sampleWall.GetHeight();
        const float baseRadius = 20.0f;
        // 三重リングに変更: 内側(0), 中間(1), 外側(2)
        const std::array<float, 3> radii = { baseRadius, baseRadius + wallHeight, baseRadius + 2.0f * wallHeight };
        const float twoPi = 2.0f * std::numbers::pi_v<float>;


        for (int ring = 0; ring < static_cast<int>(radii.size()); ++ring) {
            float radius = radii[ring];
            // Stagger every other ring so walls are not perfectly aligned radially
            float angularOffset = 0.0f;
            if (ring % 2 == 1) {

                angularOffset = (twoPi / static_cast<float>(kMaxWall_)) * 0.5f;
            }

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
            // Use filename only so ModelManager can search under rootDir (resources/model)
            e2->SetModelFile("TD_HardEnemy.obj");
            e2->SetEffectSystem(effectSystem_.get());
            enemy = e2;
        } else {
            enemy = new Enemy();
        }

        //敵の最初の生成がPlayerと被らないようにする
        const float minSpawnDist = 3.0f;
        float x = 0.0f;
        float y = 0.0f;
        for (int attempt = 0; attempt < 100; ++attempt) {
            x = Random::GeneratorFloat(-10.0f, 10.0f);
            y = Random::GeneratorFloat(-10.0f, 10.0f);
            if ((x * x + y * y) >= (minSpawnDist * minSpawnDist)) {
                break;
            }
        }

        enemy->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
        // provide player pointer so specialized enemies can chase the player
        enemy->SetPlayer(player_.get());
        enemies_.push_back(enemy);
    }

    // Also spawn an extra TwoHitEnemy together with the initial enemy wave so it's present from the first frame.
    {
        TwoHitEnemy* two = new TwoHitEnemy();
        // use project's model filename only
        two->SetModelFile("TD_HardEnemy.obj");
        const float minSpawnDist = 3.0f;
        float x = 0.0f;
        float y = 0.0f;
        for (int attempt = 0; attempt < 100; ++attempt) {
            x = Random::GeneratorFloat(-10.0f, 10.0f);
            y = Random::GeneratorFloat(-10.0f, 10.0f);
            if ((x * x + y * y) >= (minSpawnDist * minSpawnDist)) {
                break;
            }
        }
        two->SetEffectSystem(effectSystem_.get());
        two->Initialize(camera_.get(), Vector3{ x, y, 0.0f });
        two->SetPlayer(player_.get());
        enemies_.push_back(two);
    }

    // Spawn a ChaserEnemy for model draw check
    {
        ChaserEnemy* ch = new ChaserEnemy();
        // if you want a custom model file, call ch->SetModelFile("YourModel.obj");
        ch->Initialize(camera_.get(), Vector3{ 5.0f, 0.0f, 0.0f });
        ch->SetPlayer(player_.get());
        enemies_.push_back(ch);
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


    healer_ = std::make_unique<Healer>();
}

void GameScene::ModeInitialize() {
    // 既存のオブジェクト（Player, Enemy, Wall等）をすべて消す
    ClearAllObjects();

    // ゲームオーバーフラグをリセット
    isGameOver_ = false;
    gameOverState_ = GameOverState::None;
    gameOverTimer_ = 0.0f;

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
            ImGui::Text("GameOverState: %d", static_cast<int>(gameOverState_));
            ImGui::Text("GameOverTimer: %.2f", gameOverTimer_);

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