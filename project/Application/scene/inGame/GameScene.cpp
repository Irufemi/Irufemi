#include "GameScene.h"

#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"
#include "scene/SceneManager.h"

#include "actor/rock/RockManager.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "contents/GameFunction.h"

bool GameScene::s_hasPlayedTutorial_ = false;

// デストラクタ
GameScene::~GameScene() {}

// 初期化
void GameScene::Initialize(IrufemiEngine *engine) {

  // 参照したものをコピー
  // エンジン
  this->engine_ = engine;

  camera_ = std::make_unique<Camera>();
  camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
  camera_->SetTranslate(Vector3{0.0f, 10.0f, -10.0f});
  camera_->SetRotate(Vector3{-5.8f, 0.0f, 0.0f});

  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize(engine_->GetInputManager(),
                           engine_->GetClientWidth(),
                           engine_->GetClientHeight());
  debugMode = false;

  pointLight_ = std::make_unique<PointLightClass>();
  pointLight_->Initialize();
  pointLight_->SetPos(Vector3{0.0f, -5.0f, 0.0f});

  engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

  spotLight_ = std::make_unique<SpotLightClass>();
  spotLight_->Initialize();
  spotLight_->SetIntensity(0.0f);

  engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

  playerObj_ = std::make_unique<SphereClass>();
  playerObj_->Initialize(camera_.get(), "resources/texture/playerFace.png");

  player_ = std::make_unique<Player>();
  player_->Initialize(camera_.get(), playerObj_.get(),
                      Vector3{0.0f, 0.0f, 0.0f}, engine->GetInputManager());
  prevPlayerPos_ = player_->GetPosition();

  playerDeadTimer_ = 0.0f;

  enemyObj_ = std::make_unique<ObjClass>();
  enemyObj_->Initialize(camera_.get());

  Vector3 stageCenter{0.0f, 0.0f, 0.0f};
  // float stageRadius = 12.0f;

  field_.SetRadius(18.0f); // とりあえず 12
  // field_.SetHeightScale(0.15f);     // 砂丘の盛り上がり
  field_.SetFadeRates(0.75f, 1.0f); // 外周フェード

  field_.Initialize(engine, camera_.get());

  fieldFadeStarted_ = false;

  field_.ResetFade();

  enemyWallManager_.Initialize(camera_.get(), stageCenter, field_.GetRadius());
  // 敵弾マネージャ初期化
  enemyBulletManager_.SetMaxBulletCount(6); // 同時最大 6 発ぶんだけ読み込む
  enemyBulletManager_.Initialize(camera_.get(), stageCenter,
                                 field_.GetRadius());

  enemy_ = std::make_unique<Enemy>();
  enemy_->Initialize(
      camera_.get(), Vector3{0.0f, 0.0f, 7.0f}, // 敵のスポーン位置
      field_.GetRadius(), &enemyWallManager_, &enemyBulletManager_);

  // 岩の初期化
  rockManager_ = std::make_unique<RockManager>();
  rockManager_->Initialize(camera_.get()); // ← camera_ を渡す
  rockManager_->SetSpawnArea(Vector3{-10.0f, 0.0f, 5.0f},
                             Vector3{10.0f, 0.0f, 15.0f});

  // 丸フィールドを岩マネージャに教える
  rockManager_->SetField(&field_);

  skyDome_ = std::make_unique<SkyDome>();
  skyDome_->Initialize(camera_.get(), 50.0f,
                       "resources/texture/night_sky_stars.png");
  skyDome_->SetFollowCamera(true);

  // フェード
  fade_.Initialize(engine_, camera_.get());
  fade_.StartFadeIn(0.5f);

  // SEの初期化
  // hiteffect
  hitEffects_ = std::make_unique<ParticleSystem>();
  hitEffects_->Initialize(camera_.get(), "resources/gradationLine.png",
                          ParticleType::kHitEffect,
                          ParticlePrimitiveShape::Ring);
  hitEffects_->SetCull(BlendMode::kBlendModeScreen);
  hitEffects_->SetParticleColorMode(ParticleColorMode::kRed);

  // explosion
  explosion_ = std::make_unique<ParticleSystem>();
  explosion_->Initialize(camera_.get(), "resources/gradationLine.png",
                         ParticleType::kExplosion,
                         ParticlePrimitiveShape::Ring);
  explosion_->SetCull(BlendMode::kBlendModeScreen);

  // SEの初期化
  playerAttackToEnemySE_.Initialize("resources/se/player_attack_to_enemy.Mp3");
  playerAttackToWallSE_.Initialize("resources/se/player_attack_to_wall.Mp3");
  enemyAttackToPlayerSE_.Initialize("resources/se/enemy_attack_to_player.Mp3");
  playerDeadSE_.Initialize("resources/se/playerDead.Mp3");
  enemyDeadSE_.Initialize("resources/se/enemyDead.Mp3");
  cursolSE_.Initialize("resources/se/cursol.Mp3");
  decisionSE_.Initialize("resources/se/decision.Mp3");
  inGameBGM_.Initialize("resources/bgm/inGameBGM.Mp3");

  inGameBGM_.PlayFixed();

  // textureの読み込み
  engine_->GetTextureManager()->GetTextureHandle("resources/hp_bar.png");
  engine_->GetTextureManager()->GetTextureHandle("resources/hp_gauge.png");

  rockMulti_.Initialize(engine_, camera_.get());

  prevRockMultiplier_ = player_->GetMultiplier();

  tutorialRSptite_.Initialize(camera_.get(), "resources/tutorial_rock.png");
  tutorialRSptite_.SetPosition(200.0f, 80.0f);
  tutorialRSptite_.Update();

  if (!s_hasPlayedTutorial_) {
    state = GameState::Tutorial; // 初回はチュートリアル
  } else {
    state = GameState::Playing; // 2回目以降は通常プレイ
  }

 // =============================
// GameOver の UI 初期化
// =============================

  gameOverSprite_.Initialize(camera_.get(),"resources/gameover.png");
  gameOverSprite_.SetPosition( 640.0f, 220.0f ,0.0f);       // 中央上あたり
  gameOverSprite_.SetAnchor( 0.5f, 0.5f );
  

  retrySprite_.Initialize(camera_.get(),"resources/retry.png");
  retrySprite_.SetPosition( 640.0f, 460.0f,0.0f);           // 1行目
  retrySprite_.SetAnchor( 0.5f, 0.5f );
  

  titleSprite_.Initialize(camera_.get(),"resources/Totitle.png");
  titleSprite_.SetPosition(640.0f, 520.0f,0.0f);           // 2行目
  titleSprite_.SetAnchor(0.5f, 0.5f );
  
  // ベース位置を記録（スタンプ演出用）
  gameOverBasePos_ = { 640.0f, 220.0f, 0.0f };

  // =============================
  // GameClear の UI 初期化
  // =============================
  gameClearSprite_.Initialize(camera_.get(), "resources/gameclear.png");
  gameClearSprite_.SetPosition(640.0f, 220.0f, 0.0f); // 好きな位置に
  gameClearSprite_.SetAnchor(0.5f, 0.5f);
  gameClearBasePos_ = { 640.0f, 220.0f, 0.0f };

  tutorialState_ = TutorialState::Rock;
  tutorialHitEnemy_ = false;
  tutorialDamaged_ = false;
  tutorialAttackDone_ = false;

  // enemy_->SetIsTutorialRock(true);
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

  // デルタタイム
  const float deltaTime = 1.0f / 60.0f;

  switch (state) {
  case GameState::Tutorial: {

    if (debugMode) {
      debugCamera_->Update();
      camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
      camera_->SetPerspectiveFovMatrix(
          debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
      camera_->Update("Camera", player_->GetPosition(), enemy_->GetPosition());
    }

    if (skyDome_) {
      skyDome_->Update(deltaTime);
    }

    enemy_->Update(deltaTime, player_->GetPosition());

    // プレイヤーの更新処理
    player_->Update();

    rockManager_->Update(player_.get());

    field_.Update(deltaTime);

    DoCollision();

#ifdef USE_IMGUI

    ImGui::Text("isHit:%d", tutorialHitEnemy_);

#endif // USE_IMGUI

    switch (tutorialState_) {
    case TutorialState::Rock: {

      enemy_->SetIsTutorialRock(true);

      if (tutorialHitEnemy_) {
        tutorialState_ = TutorialState::Attack;
        tutorialRSptite_.Initialize(camera_.get(),
                                    "resources/tutorial_attack.png");
        tutorialRSptite_.SetPosition(220.0f, 80.0f);
        tutorialRSptite_.Update();
        // enemy_->SetIsTutorialDamage(true);
      }

      break;
    }

    case TutorialState::Attack: {
      if (tutorialAttackDone_) {
        tutorialState_ = TutorialState::Damage;

        tutorialRSptite_.Initialize(camera_.get(),
                                    "resources/tutorial_damage.png");
        tutorialRSptite_.SetPosition(40.0f, 80.0f);
        tutorialRSptite_.Update();

        enemy_->SetIsTutorialDamage(true);
      }
      break;
    }

    case TutorialState::Damage: {

      enemyWallManager_.Update(deltaTime);
      enemyBulletManager_.Update(deltaTime);

      if (player_->GetRockCount() >= 1) {
        enemy_->SetIsTutorialRock(false);
      }

      // --- 被弾して岩が減るフェーズ ---
      if (tutorialDamaged_) {

        if (player_->IsKnockback()) {
          break;
        }

        // チュートリアル終了 → 通常プレイへ
        s_hasPlayedTutorial_ = true;

        if (!fade_.IsFading() && nextSceneName_.empty()) {
          nextSceneName_ = "InGame"; // 次に行くシーン
          fade_.StartFadeOut(0.5f);  // 0.5秒フェード
        }

        tutorialRSptite_.SetColor({1.0f, 1.0f, 1.0f, 0.0f});
        tutorialRSptite_.Update();
        enemy_->SetIsTutorialDamage(false);

        // engine_->GetSceneManager()->Request("InGame");
        // state = GameState::Playing;
      }
      break;
    }
    }

    {
      int currentMul = player_->GetMultiplier();
      if (currentMul > prevRockMultiplier_) {
        // プレイヤーの少し上に出す
        Vector3 pos = player_->GetPosition();
        pos.y += 2.0f;
        rockMulti_.Show(currentMul, pos);
      }
      prevRockMultiplier_ = currentMul;
    }

    break;
  }

  case GameState::Playing:

    // カメラの更新
    if (debugMode) {
      debugCamera_->Update();
      camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
      camera_->SetPerspectiveFovMatrix(
          debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
      camera_->Update("Camera", player_->GetPosition(), enemy_->GetPosition());
    }

    // if (engine_->GetInputManager()->IsKeyPressed('P') ||
    //     engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
    //   engine_->GetSceneManager()->Request("Title");
    // }

    if (skyDome_) {
      skyDome_->Update(deltaTime);
    }

    // 壁マネージャ・弾マネージャの更新（寿命管理など）
    enemyWallManager_.Update(deltaTime);
    enemyBulletManager_.Update(deltaTime);

    // ノックバック計算用に「前フレーム位置」を記録
    prevPlayerPos_ = player_->GetPosition();

    // プレイヤーの更新処理
    player_->Update();

    // Player を Field 上に乗せて、円の内側に収める
    {
      Vector3 pos = player_->GetPosition();

      // 円の外に出ていたら、円周上にクランプ
      pos = field_.ClampInside(pos);

      player_->SetPosition(pos);
    }

    // エネミーの更新処理
    enemy_->Update(deltaTime, player_->GetPosition());

    if (enemy_->IsDead()) {
      state = GameState::EnemyDead;
      enemyDeadTimer_ = 0;

      enemyDeadPos_ = enemy_->GetPosition();

      deadCamStartPos_ = camera_->GetTranslate();
      deadCamStartFov_ = camera_->GetFovY();

      deadCamTargetFov_ = deadCamStartFov_ * 0.8f;
    }

    // ここでプレイヤー vs 壁・弾・敵本体の判定　Enemy側でやってる
    // enemy_->CheckCollisionsWithPlayer(player_.get());

#if defined USE_IMGUI
    // デバッグ：直近の当たり判定結果を表示（任意）
    // const EnemyPlayerHitResult &hit = enemy_->GetPlayerHitResult();
    // if (ImGui::Begin("HitResult")) {
    //  ImGui::Text("hitWall      : %s (index=%d)", hit.hitWall ? "true" :
    //  "false",
    //              hit.wallIndex);
    //  ImGui::Text("hitBullet    : %s (index=%d)",
    //              hit.hitBullet ? "true" : "false", hit.bulletIndex);
    //  ImGui::Text("hitEnemyBody : %s", hit.hitEnemyBody ? "true" : "false");
    //}
    // ImGui::End();
#endif

    // 岩の更新
    if (rockManager_) {
      rockManager_->Update(player_.get());
    }

    //// 岩とプレイヤーのあたり判定
    // if (player_ && rockManager_) {
    //   GameFunction::CheckHit_PlayerAndRock(*player_,
    //   rockManager_->GetRocks());
    // }

    field_.Update(deltaTime);

    // すべての当たり判定
    DoCollision();

    {
      int currentMul = player_->GetMultiplier();
      if (currentMul > prevRockMultiplier_) {
        // プレイヤーの少し上に出す
        Vector3 pos = player_->GetPosition();
        pos.y += 2.0f;
        rockMulti_.Show(currentMul, pos);
      }
      prevRockMultiplier_ = currentMul;
    }

    // particleの更新
    hitEffects_->Update();
    explosion_->Update();

    break;

  case GameState::PlayerDead: {

    inGameBGM_.Stop();

    if (playerDeadTimer_ >= 3.0f) {
      state = GameState::GameOver;
      // GAME OVER スタンプ演出開始
      gameOverStampPlaying_ = true;
      gameOverStampTimer_ = 0.0f;

      // 最初はちょっと上からスタート
      float startOffsetY = -200.0f; // 画面上方向に 200px から落とす
      gameOverSprite_.SetPosition(
          gameOverBasePos_.x,
          gameOverBasePos_.y + startOffsetY,
          gameOverBasePos_.z
      );
    }

    playerDeadTimer_ += deltaTime;
    float t = playerDeadTimer_ / playerDeadDuration_;
    if (t > 1.0f) {
      t = 1.0f;
    }

    auto Lerp = [](float a, float b, float s) { return a + (b - a) * s; };

    auto EaseOut = [](float x) {
      float inv = 1.0f - x;
      return 1.0f - inv * inv * inv * inv * inv;
    };

    // カメラにイージングを適用
    float te = EaseOut(t);

    Vector3 p = player_->GetPosition();

    // プレイヤーを少し上から・手前から見る位置を目標にする（値はあとで調整）
    deadCamTargetPos_ = {p.x, p.y + 4.0f, p.z - 8.0f};

    worldFade_ += worldFadeSpeed_ * deltaTime;
    if (worldFade_ > 1.0f) {
      worldFade_ = 1.0f;
    }

    playerDeadSE_.Play();

    field_.StartFadeToBlack(1.4f); // 1秒フェード
    fieldFadeStarted_ = true;

    // カメラの更新
    if (debugMode) {
      // デバッグ時は今まで通りデバッグカメラ優先
      debugCamera_->Update();
      camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
      camera_->SetPerspectiveFovMatrix(
          debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
      // 位置を補間
      Vector3 camPos;
      camPos.x = Lerp(deadCamStartPos_.x, deadCamTargetPos_.x, te);
      camPos.y = Lerp(deadCamStartPos_.y, deadCamTargetPos_.y, te);
      camPos.z = Lerp(deadCamStartPos_.z, deadCamTargetPos_.z, te);
      camera_->SetTranslate(camPos);

      // FOV を補間
      float fov = Lerp(deadCamStartFov_, deadCamTargetFov_, te);
      camera_->SetFovY(fov);

      // 自前で行列更新
      camera_->UpdateMatrix();
    }

    if (engine_->GetInputManager()->IsKeyPressed('P') ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
      engine_->GetSceneManager()->Request("Title");
    }

    if (skyDome_) {
      skyDome_->Update(deltaTime);
    }

    enemyWallManager_.Update(deltaTime);
    enemyBulletManager_.Update(deltaTime);

    prevPlayerPos_ = player_->GetPosition();

    // プレイヤーの更新処理
    player_->Update();

    {
      Vector3 pos = player_->GetPosition();

      pos = field_.ClampInside(pos);

      player_->SetPosition(pos);
    }

    // エネミーの更新処理
    enemy_->Update(deltaTime, player_->GetPosition());

    field_.Update(1.0f / 60.0f);
    break;
  }
  case GameState::EnemyDead: {

    inGameBGM_.Stop();
    
    if (enemyDeadTimer_ >= 3.0f) {
      state = GameState::Clear;
      enemyDeadSE_.Play();
      // GAME CLEAR スタンプ演出開始
      gameClearStampPlaying_ = true;
      gameClearStampTimer_ = 0.0f;

      float startOffsetY = -200.0f; // 画面上から落とす
      gameClearSprite_.SetPosition(
          gameClearBasePos_.x,
          gameClearBasePos_.y + startOffsetY,
          gameClearBasePos_.z
      );
    }

    enemyDeadTimer_ += deltaTime;
    float t = enemyDeadTimer_ / enemyDeadDuration_;
    if (t > 1.0f) {
      t = 1.0f;
    }

    auto Lerp = [](float a, float b, float s) { return a + (b - a) * s; };

    auto EaseOut = [](float x) {
      float inv = 1.0f - x;
      return 1.0f - inv * inv * inv * inv * inv;
    };

    // カメラにイージングを適用
    float te = EaseOut(t);

    Vector3 e = enemy_->GetPosition();
    deadCamTargetPos_ = {e.x, e.y + 4.0f, e.z - 8.0f};

    worldFade_ += worldFadeSpeed_ * deltaTime;
    if (worldFade_ > 1.0f) {
      worldFade_ = 1.0f;
    }

    // カメラの更新
    if (debugMode) {
      // デバッグ時は今まで通りデバッグカメラ優先
      debugCamera_->Update();
      camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
      camera_->SetPerspectiveFovMatrix(
          debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
      // 位置を補間
      Vector3 camPos;
      camPos.x = Lerp(deadCamStartPos_.x, deadCamTargetPos_.x, te);
      camPos.y = Lerp(deadCamStartPos_.y, deadCamTargetPos_.y, te);
      camPos.z = Lerp(deadCamStartPos_.z, deadCamTargetPos_.z, te);
      camera_->SetTranslate(camPos);

      // FOV を補間
      float fov = Lerp(deadCamStartFov_, deadCamTargetFov_, te);
      camera_->SetFovY(fov);

      // 自前で行列更新
      camera_->UpdateMatrix();
    }

    if (engine_->GetInputManager()->IsKeyPressed('P') ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
      engine_->GetSceneManager()->Request("Title");
    }

    if (skyDome_) {
      skyDome_->Update(deltaTime);
    }

    // enemyWallManager_.Update(deltaTime);
    //  enemyBulletManager_.Update(deltaTime);

    prevPlayerPos_ = player_->GetPosition();

    // プレイヤーの更新処理
    player_->Update();

    {
      Vector3 pos = player_->GetPosition();

      pos = field_.ClampInside(pos);

      player_->SetPosition(pos);
    }

    enemy_->Update(deltaTime, player_->GetPosition());

    field_.Update(1.0f / 60.0f);

    break;
  }
  case GameState::GameOver: {

     
      gameOverSprite_.Update();
      retrySprite_.Update();
      titleSprite_.Update();

      // GAME OVER スタンプ演出の更新
      if (gameOverStampPlaying_) {

          gameOverStampTimer_ += deltaTime;
          float t = gameOverStampTimer_ / gameOverStampDuration_;
          if (t >= 1.0f) {
              t = 1.0f;
              gameOverStampPlaying_ = false;
          }

          // 「上から落ちてきて ちょっとバウンド」っぽいイージング（easeOutBack）
          float s = 2.5f;   // バウンドの強さを上げる
          float u = t * t;  // ← 立ち上がりを遅くする（ゆっくり動き出す）

          float te = u - 1.0f;
          float ease = te * te * ((s + 1.0f) * te + s) + 1.0f;

          float startOffsetY = -200.0f; // 初期オフセット（Initialize 時に合わせる）
          float y = gameOverBasePos_.y + startOffsetY * (1.0f - ease);

          gameOverSprite_.SetPosition(
              gameOverBasePos_.x,
              y,
              gameOverBasePos_.z
          );
      }
#ifdef _DEBUG

    ImGui::Text("result :%d", resultIndex_);

#endif // _DEBUG

    if (engine_->GetInputManager()->IsKeyPressed('W') ||
        engine_->GetInputManager()->GetLeftStickY() > 0.0f) {
      resultIndex_ = 0;
      if (resultIndex_ < 0)
        resultIndex_ = 1;
    }
    if (engine_->GetInputManager()->IsKeyPressed('S') ||
        engine_->GetInputManager()->GetLeftStickY() < 0.0f) {
      resultIndex_ = 1;
      if (resultIndex_ > 1)
        resultIndex_ = 0;
    }

    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {

      if (resultIndex_ == 0) {
        decisionSE_.Play();
        // 0 = Retry
        engine_->GetSceneManager()->Request("InGame");
      } else if (resultIndex_ == 1) {
        decisionSE_.Play();
        // 1 = Title
        engine_->GetSceneManager()->Request("Title");
      }
    }

    //ここから追加：選択中の項目を点滅させるためのタイマー更新
    gameOverBlinkTimer_ += deltaTime;
    // 0.25秒ごとに ON / OFF
    if (gameOverBlinkTimer_ >= 0.5f) {
        gameOverBlinkTimer_ = 0.0f;
        gameOverBlinkOn_ = !gameOverBlinkOn_;
    }

    break;
  }
  case GameState::Clear: {
    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
      // engine_->GetSceneManager()->Request("Title");
    }

	gameClearSprite_.Update();  
	titleSprite_.Update();

    //Title 選択肢点滅用タイマー（GameOver と共通）
    gameOverBlinkTimer_ += deltaTime;
    if (gameOverBlinkTimer_ >= 0.5f) {   // 0.25秒ごとに ON/OFF
        gameOverBlinkTimer_ = 0.0f;
        gameOverBlinkOn_ = !gameOverBlinkOn_;
    }

    // GAME CLEAR スタンプ演出更新
    if (gameClearStampPlaying_) {

        gameClearStampTimer_ += deltaTime;
        float t = gameClearStampTimer_ / gameClearStampDuration_;
        if (t >= 1.0f) {
            t = 1.0f;
            gameClearStampPlaying_ = false;
        }

        // ゆっくり始まりつつ、ドンッと押される easeOutBack 系
        float s = 2.3f;      // バウンドの強さ
        float u = t * t;     // 立ち上がりを遅くする
        float te = u - 1.0f;
        float ease = te * te * ((s + 1.0f) * te + s) + 1.0f;

        float startOffsetY = -200.0f;
        float y = gameClearBasePos_.y + startOffsetY * (1.0f - ease);

        gameClearSprite_.SetPosition(
            gameClearBasePos_.x,
            y,
            gameClearBasePos_.z
        );
    }

    // fade_.Update(deltaTime);

    // if (!fade_.IsFading()) {
    //   nextSceneName_ = "Title";
    //   fade_.StartFadeOut(0.5f);
    // }

    //// フェードアウト完了後に切り替えたい場合は
    // if (!fade_.IsFading() && !nextSceneName_.empty()) {
    //   engine_->GetSceneManager()->Request(nextSceneName_.c_str());
    // }

    if (!fade_.IsFading()) {
      if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) ||
          engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {

        // 次に行くシーン名をセットして、フェードアウト開始
        nextSceneName_ = "Title";
        fade_.StartFadeOut(0.5f);
      }
    }

    break;
  }
  }

  rockMulti_.Update(deltaTime);

  // rockMulti_.Show(prevRockMultiplier_,player_->GetPosition());

  fade_.Update(deltaTime);

  if (!fade_.IsFading() && !nextSceneName_.empty()) {
    engine_->GetSceneManager()->Request(nextSceneName_.c_str());
    nextSceneName_.clear();
  }
}

// 描画
void GameScene::Draw() {

  Vector4 worldDarkColor = GetWorldDarkColor();

  // 3D
  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);

  // ★ 天球
  if (skyDome_) {
    engine_->ApplySkyDomePSO();
    // 背景なのでデプス書き込みOFF
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);

    skyDome_->Draw();

    // 以降のオブジェクト用に戻す
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
  }

  engine_->ApplyPSO();

  // プレイヤーの描画処理
  player_->Draw();

  // エネミーの描画処理（中で壁・弾も描画）
  enemy_->Draw();

  // ここでフィールド専用 PSO を反映
  engine_->ApplyFieldCylinderPSO();

  // 地面（ステージ）
  field_.Draw();

  // Region
  engine_->ApplyRegionPSO();

  // 敵の壁と弾を描画（Region を使う）
  enemyWallManager_.Draw();
  enemyBulletManager_.Draw();

  // 岩の描画
  if (rockManager_) {
    rockManager_->Draw(engine_, camera_.get());
  }

  // Particle
  engine_->SetBlend(BlendMode::kBlendModeAdd);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
  engine_->ApplyParticlePSO();

  hitEffects_->Draw();
  explosion_->Draw();

  // Sprite

  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
  engine_->ApplySpritePSO();

  if (state == GameState::Tutorial) {
    tutorialRSptite_.Draw();
  }

  rockMulti_.Draw();

  fade_.Draw();

  // =============================
  // GameOver UI の描画
  // =============================
  if (state == GameState::GameOver) {

      // 通常色（白）
      Vector4 normalColor{ 1.0f, 1.0f, 1.0f, 1.0f };

      // 点滅時の色（明るい黄色と暗めの黄色）
      Vector4 selectedOnColor{ 1.0f, 1.0f, 0.0f, 1.0f };  // 光ってる状態
      Vector4 selectedOffColor{ 0.4f, 0.4f, 0.0f, 1.0f };  // ちょっと暗く

      // いったん全体を通常色にリセット
      retrySprite_.SetColor(normalColor);
      titleSprite_.SetColor(normalColor);

      // 今の点滅状態に応じた色
      Vector4 blinkColor = gameOverBlinkOn_ ? selectedOnColor : selectedOffColor;

      // 選択中の方だけ点滅色にする
      if (resultIndex_ == 0) {
          // Retry 選択中
          retrySprite_.SetColor(blinkColor);
      }
      else {
          // Title 選択中
          titleSprite_.SetColor(blinkColor);
      }

      gameOverSprite_.Draw();
      retrySprite_.Draw();
      titleSprite_.Draw();
  }

  // =============================
  // GameClear UI の描画
  // =============================
  if (state == GameState::Clear) {

      // Title 選択肢を点滅させる
      Vector4 onColor{ 1.0f, 1.0f, 0.0f, 1.0f }; // 光ってる黄色
      Vector4 offColor{ 0.4f, 0.4f, 0.0f, 1.0f }; // 暗め黄色

      Vector4 blinkColor = gameOverBlinkOn_ ? onColor : offColor;

      titleSprite_.SetColor(blinkColor);
      titleSprite_.Draw();
  }

}

void GameScene::DoCollision() {

  // if (player_ && rockManager_) {
  //   GameFunction::CheckHit_PlayerAndRock(*player_,
  //   rockManager_->GetRocks());
  // }

  if (!player_ || !enemy_) {
    return;
  }

  if (!player_->GetIsAlive()) {
    return;
  }

  if (enemy_->GetInPhaseTransition()) {
    return;
  }

  const auto &pPos = player_->GetPosition();
  float pRadius = player_->GetRadius();

  // -------- プレイヤー vs 壁 --------
  {
    int wallIndex = enemyWallManager_.CheckCollisionCircle(pPos, pRadius);
    if (wallIndex >= 0) {
      // 壁側の処理（壊す・状態変更など）
      enemyWallManager_.OnPlayerHitWall(wallIndex);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);

      if (!player_->IsInvincible()) {

        if (player_->GetRockCount() >= 1) {
          playerAttackToEnemySE_.Play();
        }

        // 岩0で当たると死亡
        // if (player_->GetRockCount() <= 0) {
        //  player_->Dead();

        //  if (state == GameState::Playing) {
        //    state = GameState::PlayerDead;
        //    playerDeadTimer_ = 0.0f;

        //    // カメラ寄せの開始情報を記録
        //    deadCamStartPos_ = camera_->GetTranslate();
        //    deadCamStartFov_ = camera_->GetFovY();

        //    // Vector3 p = player_->GetPosition();

        //    ////
        //    ///
        //    プレイヤーを少し上から・手前から見る位置を目標にする（値はあとで調整）
        //    // playerDeadCamTargetPos_ = {p.x, p.y + 3.0f, p.z - 8.0f};

        //    // FOV は少しだけ狭めて寄ってる感じに
        //    deadCamTargetFov_ = deadCamStartFov_ * 0.8f;
        //  }
        //}

        int before = player_->GetRockCount();

        // ノックバックと岩のリセット
        player_->HalveRockCount();

        if (state == GameState::Tutorial &&
            tutorialState_ == TutorialState::Damage) {
          tutorialDamaged_ = true;
        }

        int after = player_->GetRockCount();
        int numToDetach = before - after;
        int spawnCount = (numToDetach + 1) / 2;

        // 見た目の岩も外す
        if (rockManager_) {

          // 外す対象を取得
          auto detached = rockManager_->SelectDetachedRocks(numToDetach);

          // ノックバック方向を取得する関数にする
          Vector3 knockDir = ApplyPlayerKnockback(0.8f);

          // 散らばす
          rockManager_->SpawnDroppedRocks(detached, spawnCount,
                                          player_->GetPosition(), knockDir);
          // rockManager_->HalveAttachedRocks(numToDetach);
        }

        // 無敵開始
        player_->StartInvincible(45);
      }
    }
  }

  // -------- プレイヤー vs 弾 --------
  {
    int bulletIndex = enemyBulletManager_.CheckCollisionCircle(pPos, pRadius);
    if (bulletIndex >= 0) {
      // 弾側の処理（消すなど）

      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);
      hitEffects_->PlayHitEffect(pPos);

      if (!player_->IsInvincible()) {

        if (player_->GetRockCount() >= 1) {
          playerAttackToEnemySE_.Play();
        }

        // 岩0で当たると死亡
        if (player_->GetRockCount() <= 0) {
          player_->Dead();
          enemyAttackToPlayerSE_.Play();
          if (state == GameState::Playing) {
            state = GameState::PlayerDead;
            playerDeadTimer_ = 0.0f;

            // カメラ寄せの開始情報を記録
            deadCamStartPos_ = camera_->GetTranslate();
            deadCamStartFov_ = camera_->GetFovY();

            // Vector3 p = player_->GetPosition();

            ////
            /// プレイヤーを少し上から・手前から見る位置を目標にする（値はあとで調整）
            // playerDeadCamTargetPos_ = {p.x, p.y + 3.0f, p.z - 8.0f};

            // FOV は少しだけ狭めて寄ってる感じに
            deadCamTargetFov_ = deadCamStartFov_ * 0.8f;
          }
        }

        enemyBulletManager_.OnHitBullet(bulletIndex);

        int before = player_->GetRockCount();

        // ノックバックと岩のリセット
        player_->HalveRockCount();

        if (state == GameState::Tutorial &&
            tutorialState_ == TutorialState::Damage) {
          tutorialDamaged_ = true;
        }

        int after = player_->GetRockCount();
        int numToDetach = before - after;
        int spawnCount = (numToDetach + 1) / 2;

        // 見た目の岩も外す
        if (rockManager_) {
          // rockManager_->HalveAttachedRocks(numToDetach);

          // 外す対象を取得
          auto detached = rockManager_->SelectDetachedRocks(numToDetach);

          // ノックバック方向を取得する関数にする
          Vector3 knockDir = ApplyPlayerKnockback(0.8f);

          // 散らばす
          rockManager_->SpawnDroppedRocks(detached, spawnCount,
                                          player_->GetPosition(), knockDir);
        }

        // 無敵開始
        player_->StartInvincible(45);
      }
    }
  }

  // -------- プレイヤー vs 敵本体 --------
  {
    // 潜っている間（BurrowHidden）は本体当たり判定を取らない
    if (!enemy_->IsBurrowing()) {
      const Vector3 &ePos = enemy_->GetPosition();
      float eRadius = enemy_->GetRadius();

      // 当たり判定を少し緩くする
      float hitPlayerRadius = pRadius * 0.9f;

      if (GameFunction::IsHitCircleRect(pPos, hitPlayerRadius, ePos,
                                        eRadius * 2.0f, eRadius * 2.0f)) {

        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);
        hitEffects_->PlayHitEffect(pPos);

        if (!player_->IsInvincible()) {

          if (player_->GetRockCount() >= 1) {
            playerAttackToEnemySE_.Play();
          }

          // 岩0で当たると死亡
          if (player_->GetRockCount() <= 0) {

            if (state != GameState::Tutorial) {
              player_->Dead();

              enemyAttackToPlayerSE_.Play();
              if (state == GameState::Playing) {
                state = GameState::PlayerDead;
                playerDeadTimer_ = 0.0f;

                // カメラ寄せの開始情報を記録
                deadCamStartPos_ = camera_->GetTranslate();
                deadCamStartFov_ = camera_->GetFovY();

                // Vector3 p = player_->GetPosition();

                ////
                /// プレイヤーを少し上から・手前から見る位置を目標にする（値はあとで調整）
                // playerDeadCamTargetPos_ = {p.x, p.y + 3.0f, p.z - 8.0f};

                // FOV は少しだけ狭めて寄ってる感じに
                deadCamTargetFov_ = deadCamStartFov_ * 0.8f;
              }
            }
          }

          // すでにノックバック中なら、これ以上当たり判定しない
          if (!player_->IsKnockback()) {

            // 敵にダメージ
            enemy_->ApplyDamageFromPlayer(player_->GetAttackPower());
            explosion_->PlayExplosion(pPos);

            if (state == GameState::Tutorial) {
              if (tutorialState_ == TutorialState::Rock) {
                if (player_->GetRockCount() >= 1) {
                  tutorialHitEnemy_ = true;
                }
              } else if (tutorialState_ == TutorialState::Attack) {
                if (player_->GetRockCount() >= 3) {
                  tutorialAttackDone_ = true;
                }
              }
            }

            // スタン開始
            enemy_->StartStan(20);

            if (enemy_->GetState() == EnemyState::DashForward) {

              int before = player_->GetRockCount();

              // ノックバックと岩のリセット
              player_->HalveRockCount();

              // 無敵開始
              player_->StartInvincible(90);

              if (state == GameState::Tutorial &&
                  tutorialState_ == TutorialState::Damage) {
                tutorialDamaged_ = true;
              }

              int after = player_->GetRockCount();
              int numToDetach = before - after;
              int spawnCount = (numToDetach + 1) / 2;

              // 見た目の岩も外す
              if (rockManager_) {
                // rockManager_->HalveAttachedRocks(numToDetach);

                // 外す対象を取得
                auto detached = rockManager_->SelectDetachedRocks(numToDetach);

                // ノックバック方向を取得する関数にする
                Vector3 knockDir = ApplyPlayerKnockback(0.8f);

                // 散らばす
                rockManager_->SpawnDroppedRocks(
                    detached, spawnCount, player_->GetPosition(), knockDir);
              }

            } else {

              // プレイヤーの岩をリセット
              player_->ResetRockCount();

              // 見た目の岩も外す
              if (rockManager_) {
                rockManager_->ResetAttachedRocks();
              }
            }

            // 突進してきている場合は、ここで突進を止める
            enemy_->ForceStopDash();

            // ノックバック
            ApplyPlayerKnockback(1.0f);

            // 無敵開始
            // player_->StartInvincible(45);

            // カメラのシェイク
            if (camera_) {
              camera_->StartShake();
            }
          }
        }
      }
    }
  }
}

Vector3 GameScene::ApplyPlayerKnockback(const float knockbackPower) {
  if (!player_) {
    return {0.0f, 0.0f, 0.0f};
  }

  Vector3 p = player_->GetPosition();
  Vector3 e = enemy_ ? enemy_->GetPosition() : p;

  // ノックバック方向 = 相手 → プレイヤー の方向
  Vector3 dir{p.x - e.x, 0.0f, p.z - e.z};

  float len = dir.x * dir.x + dir.z * dir.z;
  if (len > 0.0001f) {
    len = std::sqrt(len);
    dir.x /= len;
    dir.z /= len;
  } else {
    dir = {0.0f, 0.0f, 1.0f};
  }

  // 縦方向上昇
  player_->AddVerticalVelocity(0.7f);

  // ノックバック開始
  player_->StartKnockback(dir, knockbackPower);

  return dir;
}
