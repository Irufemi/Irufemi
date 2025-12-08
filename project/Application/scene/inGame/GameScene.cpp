#include "GameScene.h"

#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"
#include "scene/SceneManager.h"

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
  camera_->SetTranslate(Vector3{0.0f, 5.0f, -10.0f});
  camera_->SetRotate(Vector3{-6.0f, 0.0f, 0.0f});

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

  enemyObj_ = std::make_unique<SphereClass>();
  enemyObj_->Initialize(camera_.get());

  Vector3 stageCenter{0.0f, 0.0f, 0.0f};
  float stageRadius = 20.0f;

  enemyWallManager_.Initialize(camera_.get(), stageCenter, stageRadius);
  enemyBulletManager_.Initialize(camera_.get(), stageCenter, stageRadius);

  enemy_ = std::make_unique<Enemy>();
  enemy_->Initialize(camera_.get(),
                     Vector3{0.0f, 0.0f, 7.0f}, // 敵のスポーン位置
                     stageRadius, &enemyWallManager_, &enemyBulletManager_);

  // 岩の初期化
  rockManager_ = std::make_unique<RockManager>();
  rockManager_->Initialize(camera_.get()); // ← camera_ を渡す
  rockManager_->SetSpawnArea(Vector3{-10.0f, 0.0f, 5.0f},
                             Vector3{10.0f, 0.0f, 15.0f});
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
    camera_->SetPerspectiveFovMatrix(
        debugCamera_->GetCamera().GetPerspectiveFovMatrix());
  } else {
    camera_->Update("Camera", player_->GetPosition(), enemy_->GetPosition());
  }

  if (engine_->GetInputManager()->IsKeyPressed('P') ||
      engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
    engine_->GetSceneManager()->Request("Title");
  }

  // 仮のデルタタイム
  // エンジンに GetDeltaTime() があるなら、それを使って置き換えてOK
  const float deltaTime = 1.0f / 60.0f;

  // 壁マネージャ・弾マネージャの更新（寿命管理など）
  enemyWallManager_.Update(deltaTime);
  enemyBulletManager_.Update(deltaTime);

  // ノックバック計算用に「前フレーム位置」を記録
  prevPlayerPos_ = player_->GetPosition();

  // プレイヤーの更新処理
  player_->Update();

  // エネミーの更新処理
  enemy_->Update(deltaTime, player_->GetPosition());

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
  //   GameFunction::CheckHit_PlayerAndRock(*player_, rockManager_->GetRocks());
  // }

  // すべての当たり判定
  DoCollision();
}

// 描画
void GameScene::Draw() {

  // 3D
  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
  engine_->ApplyPSO();

  // プレイヤーの描画処理
  player_->Draw();

  // エネミーの描画処理（中で壁・弾も描画）
  enemy_->Draw();

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

  if (player_ && rockManager_) {
    GameFunction::CheckHit_PlayerAndRock(*player_, rockManager_->GetRocks());
  }

  if (!player_ || !enemy_) {
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

        int before = player_->GetRockCount();

        // ノックバックと岩のリセット
        player_->HalveRockCount();

        int after = player_->GetRockCount();
        int numToDetach = before - after;

        // 見た目の岩も外す
        if (rockManager_) {
          rockManager_->HalveAttachedRocks(numToDetach);
        }

        // ノックバック
        ApplyPlayerKnockback();

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

        enemyBulletManager_.OnHitBullet(bulletIndex);

        int before = player_->GetRockCount();

        // ノックバックと岩のリセット
        player_->HalveRockCount();

        int after = player_->GetRockCount();
        int numToDetach = before - after;

        // 見た目の岩も外す
        if (rockManager_) {
          rockManager_->HalveAttachedRocks(numToDetach);
        }

        // ノックバック
        ApplyPlayerKnockback();

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

      if (GameFunction::isHitCircle(pPos, hitPlayerRadius, ePos, eRadius)) {

        if (!player_->IsInvincible()) {

          // すでにノックバック中なら、これ以上当たり判定しない
          if (!player_->IsKnockback()) {

            // 突進してきている場合は、ここで突進を止める
            enemy_->ForceStopDash();

            // プレイヤーの岩をリセット
            player_->ResetRockCount();

            // 見た目の岩も外す
            if (rockManager_) {
              rockManager_->ResetAttachedRocks();
            }

            // 敵にダメージ
            enemy_->ApplyDamageFromPlayer(player_->GetAttackPower());

            // ノックバック
            ApplyPlayerKnockback();

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

void GameScene::ApplyPlayerKnockback() {
  if (!player_) {
    return;
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
  player_->StartKnockback(dir);
}
