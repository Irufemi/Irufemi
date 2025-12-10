#include "GameScene.h"

#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"
#include "scene/SceneManager.h"

#include "actor/rock/RockManager.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "contents/GameFunction.h"
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
  playerObj_->Initialize(camera_.get());

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

  enemyWallManager_.Initialize(camera_.get(), stageCenter, field_.GetRadius());
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
  skyDome_->Initialize(camera_.get(), 50.0f, "resources/texture/night_sky_stars.png");
  skyDome_->SetFollowCamera(true);
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

    if (engine_->GetInputManager()->IsKeyPressed('P') ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
      engine_->GetSceneManager()->Request("Title");
    }

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

    break;

  case GameState::PlayerDead: {

    if (playerDeadTimer_ >= 3.0f) {
      state = GameState::GameOver;
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

    if (enemyDeadTimer_ >= 5.0f) {
      state = GameState::Clear;
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

    Vector3 e = enemyDeadPos_;
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

#ifdef _DEBUG

    ImGui::Text("result :%d", resultIndex_);

#endif // _DEBUG

    if (engine_->GetInputManager()->IsKeyPressed('W')) {
      resultIndex_--;
      if (resultIndex_ < 0)
        resultIndex_ = 1;
    }
    if (engine_->GetInputManager()->IsKeyPressed('S')) {
      resultIndex_++;
      if (resultIndex_ > 1)
        resultIndex_ = 0;
    }

    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE)) {

      if (resultIndex_ == 0) {
        // 0 = Retry
        engine_->GetSceneManager()->Request("InGame");
      } else if (resultIndex_ == 1) {
        // 1 = Title
        engine_->GetSceneManager()->Request("Title");
      }
    }

    break;
  }
  case GameState::Clear: {
    if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) ||
        engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
      engine_->GetSceneManager()->Request("Title");
    }
    break;
  }
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

void GameScene::DoCollision() {

  // if (player_ && rockManager_) {
  //   GameFunction::CheckHit_PlayerAndRock(*player_, rockManager_->GetRocks());
  // }

  if (!player_ || !enemy_) {
    return;
  }

  if (!player_->GetIsAlive()) {
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

      if (!player_->IsInvincible()) {

        // 岩0で当たると死亡
        if (player_->GetRockCount() <= 0) {
          player_->Dead();

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

        int before = player_->GetRockCount();

        // ノックバックと岩のリセット
        player_->HalveRockCount();

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

      if (!player_->IsInvincible()) {

        // 岩0で当たると死亡
        if (player_->GetRockCount() <= 0) {
          player_->Dead();
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

        int after = player_->GetRockCount();
        int numToDetach = before - after;
        int spawnCount = (numToDetach + 1) / 2;

        // 見た目の岩も外す
        if (rockManager_) {
          rockManager_->HalveAttachedRocks(numToDetach);

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

        if (!player_->IsInvincible()) {

          // 岩0で当たると死亡
          if (player_->GetRockCount() <= 0) {
            player_->Dead();
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

          // すでにノックバック中なら、これ以上当たり判定しない
          if (!player_->IsKnockback()) {

            // 敵にダメージ
            enemy_->ApplyDamageFromPlayer(player_->GetAttackPower());

            if (enemy_->GetState() == EnemyState::DashForward) {

              int before = player_->GetRockCount();

              // ノックバックと岩のリセット
              player_->HalveRockCount();

              int after = player_->GetRockCount();
              int numToDetach = before - after;
              int spawnCount = (numToDetach + 1) / 2;

              // 見た目の岩も外す
              if (rockManager_) {
                rockManager_->HalveAttachedRocks(numToDetach);

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
            player_->StartInvincible(45);

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

  // 現在位置と、Update 前に記録しておいた位置との差分
  Vector3 currentPos = player_->GetPosition();
  Vector3 moveVec{currentPos.x - prevPlayerPos_.x, 0.0f,
                  currentPos.z - prevPlayerPos_.z};

  // XZ 平面の移動量の長さ²
  float lenSq = moveVec.x * moveVec.x + moveVec.z * moveVec.z;

  float len = std::sqrt(lenSq);

  // ノックバックさせる方向
  Vector3 dir{};

  if (lenSq >= 0.0001f) {
    // 十分動いていれば、その逆向きに飛ばす
    float len = std::sqrt(lenSq);
    dir = {-moveVec.x / len, 0.0f, -moveVec.z / len};
  } else {
    // ほぼ動いていない → 弾・敵側からの押し出し方向でノックバック

    if (enemy_) {
      Vector3 fromEnemy{currentPos.x - enemy_->GetPosition().x, 0.0f,
                        currentPos.z - enemy_->GetPosition().z};

      float lenSq2 = fromEnemy.x * fromEnemy.x + fromEnemy.z * fromEnemy.z;

      if (lenSq2 >= 0.0001f) {
        float len2 = std::sqrt(lenSq2);
        dir = {fromEnemy.x / len2, 0.0f, fromEnemy.z / len2};
      } else {
        // それでも方向が決められない場合は適当な方向に飛ばす
        dir = {0.0f, 0.0f, 1.0f};
      }
    } else {
      // 敵がいない状況用の保険
      dir = {0.0f, 0.0f, 1.0f};
    }
  }

  // 縦方向の初速（跳ね上がり）
  player_->AddVerticalVelocity(0.7f);

  // ノックバック開始
  player_->StartKnockback(dir, knockbackPower);

  return dir;
}
