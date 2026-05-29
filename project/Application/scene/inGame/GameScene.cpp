#include "GameScene.h"
#include <algorithm>
#include <format>

#include "Framework/SceneManager.h"
#include "Irufemi.h"
#include <dinput.h>

#include "Graphics/Data/AreaLight.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Graphics/Camera/DebugCamera.h"

#include "Graphics/PostProcess/PostProcessManager.h"
#include "actors/enemy/Enemy.h"
#include "actors/enemy/Body/Body.h"
#include "actors/enemy/EnemyParameters.h"
#include "actors/player/Player.h"
#include "contents/field/Field.h"
#include "contents/field/building/building.h"
#include "contents/skydome/Skydome.h"
#include "contents/ui/EnemyHPBar.h"
#include "contents/ui/EnemyPartHPBar.h"
#include "contents/ui/PlayerHPBar.h"
#include "contents/light/DynamicArenaLight.h"
#include "Renderer/Object2D/Sprite/Sprite.h"

#include "Engine/Core/Math/Geometry/Collision.h"

#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/IrufemiEngine.h"
#include <Windows.h>

namespace {
// --- ローカル定数（座標やベクトルなどの初期値） ---
const Vector3 kDefaultCameraPos = {0.0f, 0.0f, -10.0f};
const Vector3 kDefaultLightDir = {0.0f, -1.0f, 1.0f};
const Vector3 kDefaultNormalZ = {0.0f, 0.0f, 1.0f};
} // namespace

float GameScene::clearTime_ = 0.0f;

GameScene::GameScene() {}

GameScene::~GameScene() {}

void GameScene::Initialize(IrufemiEngine *engine) {
  BaseScene::Initialize(engine);

  clearTime_ = 0.0f;

  bgm_ = std::make_unique<Bgm>();
  bgm_->Initialize("resources/BGM/InGame.mp3", "InGameBGM", true, true);

  Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
  activeCamera->SetTranslate(kDefaultCameraPos);
  activeCamera->UpdateMatrix();

  // プレイヤーの初期化
  player_ = std::make_unique<Player>();
  player_->Initialize(engine_->GetInputManager(), engine_);

  boss_ = std::make_unique<Enemy>();
  boss_->Initialize(engine_);

  field_ = std::make_unique<Field>(engine_);
  field_->Initialize();

  skydome_ = std::make_unique<Skydome>();
  skydome_->Initialize();

  dynamicArenaLight_ = std::make_unique<DynamicArenaLight>();
  dynamicArenaLight_->Initialize(engine_, areaLights_);

  // 操作説明スプライトの初期化 (1人称視点用)
  float screenW = static_cast<float>(engine_->GetClientWidth());
  float screenH = static_cast<float>(engine_->GetClientHeight());
  auto initKeyUI = [&](std::unique_ptr<Sprite>& sprite, const std::string& path, float x, float y) {
      sprite = std::make_unique<Sprite>();
      sprite->Initialize(path);
      sprite->SetSize(300.0f, 50.0f);
      sprite->SetPositionTopLeft(x, y);
      sprite->SetColor({0.8f, 0.8f, 0.8f, 1.0f});
  };

  float uiBaseX = 20.0f;
  initKeyUI(uiLClickNormal_, "resources/texture/inGame/ui_lclick_normal.png", uiBaseX, screenH - 320.0f);
  initKeyUI(uiLClickCharged_, "resources/texture/inGame/ui_lclick_charged.png", uiBaseX, screenH - 320.0f);
  initKeyUI(uiRClickNormal_, "resources/texture/inGame/ui_rclick_normal.png", uiBaseX, screenH - 260.0f);
  initKeyUI(uiRClickCharged_, "resources/texture/inGame/ui_rclick_charged.png", uiBaseX, screenH - 260.0f);
  initKeyUI(uiE_, "resources/texture/inGame/ui_e.png", uiBaseX, screenH - 200.0f);
  initKeyUI(uiV_, "resources/texture/inGame/ui_v.png", uiBaseX, screenH - 140.0f);
  initKeyUI(uiSpace_, "resources/texture/inGame/ui_space.png", uiBaseX, screenH - 80.0f);

  cooldownWarningSprite_ = std::make_unique<Sprite>();
  cooldownWarningSprite_->Initialize("resources/texture/inGame/cooldown_warning.png");
  cooldownWarningSprite_->SetSize(400.0f, 100.0f);
  cooldownWarningSprite_->SetPositionCenter(static_cast<float>(engine_->GetClientWidth()) / 2.0f + 15.0f, static_cast<float>(engine_->GetClientHeight()) / 2.0f + 80.0f);

  keyEscSprite_ = std::make_unique<Sprite>();
  keyEscSprite_->Initialize("resources/texture/inGame/key_ESC.png");
  keyEscSprite_->SetAnchor(0.0f, 0.0f);
  keyEscSprite_->SetPosition(20.0f, 20.0f);
  keyEscSprite_->SetSize(64.0f, 64.0f);
  keyEscSprite_->SetColor({0.8f, 0.8f, 0.8f, 1.0f});
}

void GameScene::Update() {
#if defined(_DEBUG) || defined(DEVELOPMENT)
  if (PressedDIK(kKeyDebugCameraToggle)) {
    isDebugCameraMode_ = !isDebugCameraMode_;
    if (isDebugCameraMode_ && isFirstDebug_) {
      debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *engine_->GetCameraManager()->GetActiveCamera());
      isFirstDebug_ = false;
    }
  }
#endif

  // ポーズ画面呼び出し
  InputManager* input = engine_->GetInputManager();
  if (input && (input->IsKeyPressed(VK_ESCAPE) || input->StartPressed())) {
      engine_->GetSceneManager()->PushScene("Pause");
      return;
  }

  // =====
  // ↓ゲームの更新
  // =====

  // プレイヤーの更新
  if (player_ && !isDebugCameraMode_) {
    // ボスの座標を毎フレーム教える
    if (boss_) {
      Vector3 bossPos = boss_->GetTargetPosition();
      player_->SetTargetPosition(bossPos);

      // ボスが画面（プレイヤーの前方）にいるかどうかの判定
      Vector3 playerPos = player_->GetTranslate();
      Vector3 toBoss = Math::Subtract(bossPos, playerPos);
      float len = Math::Length(toBoss);
      bool inScreen = false;

      if (len > kMathEpsilon) {
        toBoss = {toBoss.x / len, toBoss.y / len, toBoss.z / len};
        float sinY = std::sin(player_->GetRotate().y);
        float cosY = std::cos(player_->GetRotate().y);
        Vector3 playerForward = {sinY, 0.0f, cosY};
        float dot = Math::Dot(toBoss, playerForward);
        if (dot > 0.0f)
          inScreen = true;
      }
      player_->SetIsTargetingEnemy(inScreen);
    }
    player_->Update();
  }

  if (boss_) {
    if (!boss_->IsDead()) {
      clearTime_ += engine_->GetDeltaTime();
    }
    boss_->Update(player_.get());

    if (boss_->IsDead() && !isDeathCameraMode_) {
        // 死亡した瞬間にプレイヤーの操作を停止し、カメラ演出を開始
        if (player_) player_->SetCinematicMode(true);
        isDeathCameraMode_ = true;
        deathCameraLerpTimer_ = 0.0f;
        
        // 死亡演出開始時にビルの描画を非表示にする
        if (field_) {
            field_->SetDrawBuildings(false);
        }
        
        Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
        if (activeCamera) {
            initialCameraPos_ = activeCamera->GetTranslate();
            Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(activeCamera->GetRotate());
            Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
            initialCameraTarget_ = Math::Add(initialCameraPos_, Math::Multiply(50.0f, forward));
        }
    }
  }

  // ボス死亡中のカメラ演出更新
  if (isDeathCameraMode_ && !isDebugCameraMode_) {
      if (player_) player_->ClearMissiles();
      deathCameraLerpTimer_ += engine_->GetDeltaTime();
      float t = deathCameraLerpTimer_ / 4.0f; // 約4.0秒かけてズームイン
      if (t > 1.0f) t = 1.0f;
      float easedT = t * t * (3.0f - 2.0f * t);

      Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
      if (activeCamera && boss_ && player_) {
          Vector3 bossTarget = boss_->GetTargetPosition();
          Vector3 playerPos = player_->GetTranslate();

          // プレイヤーからボスへの水平ベクトルを算出
          Vector3 playerToBoss = Math::Subtract(bossTarget, playerPos);
          playerToBoss.y = 0.0f;
          float dist = Math::Length(playerToBoss);
          if (dist < 0.1f) playerToBoss = {0.0f, 0.0f, 1.0f};
          else playerToBoss = Math::Normalize(playerToBoss);

          if (dist < kTargetPlayerBossDistance) {
              float pullAmount = kTargetPlayerBossDistance - dist;
              float moveStep = kPlayerBackoffSpeed * engine_->GetDeltaTime();
              if (moveStep > pullAmount) {
                  moveStep = pullAmount;
              }
              // playerToBoss の逆方向（後退方向）へ移動
              Vector3 backPos = Math::Subtract(playerPos, Math::Multiply(moveStep, playerToBoss));
              player_->SetTranslate(backPos);

              // 以降のカメラ計算用に位置と向きベクトルを再計算
              playerPos = backPos;
              playerToBoss = Math::Subtract(bossTarget, playerPos);
              playerToBoss.y = 0.0f;
              dist = Math::Length(playerToBoss);
              if (dist < 0.1f) playerToBoss = {0.0f, 0.0f, 1.0f};
              else playerToBoss = Math::Normalize(playerToBoss);
          }

          // カメラの引き距離を死亡演出タイマーの経過時間に応じて動的に変化させる！
          // 合体完了（4.5秒経過）した瞬間に、32.0f から 12.0f まで超高速ズームインし、その後爆散前に引きへスッと戻る
          float currentBehindDist = kCameraBehindDistance; // 通常引き 32.0f
          
          if (deathCameraLerpTimer_ > 4.5f) {
              float afterGatherTime = deathCameraLerpTimer_ - 4.5f; // 合体完了からの経過時間 (0.0s 〜 2.0s)
              
              if (afterGatherTime < 0.7f) {
                  // 合体後0.0s〜0.7s: カメラを 32.0f から 12.0f へ急接近（高速クローズアップ）！
                  float zoomT = afterGatherTime / 0.7f;
                  float easedZoomT = zoomT * zoomT * (3.0f - 2.0f * zoomT);
                  currentBehindDist = kCameraBehindDistance + (12.0f - kCameraBehindDistance) * easedZoomT;
              } else {
                  // 合体後0.7s〜2.0s: カメラを 12.0f から 32.0f の元の引きへ戻す（ズームバック）！
                  float zoomBackT = (afterGatherTime - 0.7f) / 1.3f;
                  if (zoomBackT > 1.0f) zoomBackT = 1.0f;
                  float easedZoomBackT = zoomBackT * zoomBackT * (3.0f - 2.0f * zoomBackT);
                  currentBehindDist = 12.0f + (kCameraBehindDistance - 12.0f) * easedZoomBackT;
              }
          }

          // カメラ目標位置を「プレイヤーの背後」かつ「少し見上げる高さ」に配置
          Vector3 targetCamPos = Math::Subtract(playerPos, Math::Multiply(currentBehindDist, playerToBoss));
          targetCamPos.y = playerPos.y + kCameraHeightOffset;
          if (targetCamPos.y < kGroundClampMinY) {
              targetCamPos.y = kGroundClampMinY;
          }

          // 注視点はボスの胴体と頭部の中間付近（高さ+6.0f）に設定
          Vector3 lookAtTarget = bossTarget;
          lookAtTarget.y += kBossLookAtHeightOffset;

          Vector3 currentPos = Math::Add(initialCameraPos_, Math::Multiply(easedT, Math::Subtract(targetCamPos, initialCameraPos_)));
          Vector3 currentTarget = Math::Add(initialCameraTarget_, Math::Multiply(easedT, Math::Subtract(lookAtTarget, initialCameraTarget_)));
          
          activeCamera->SetTranslate(currentPos);
          
          Vector3 forward = Math::Subtract(currentTarget, currentPos);
          float forwardDist = Math::Length(forward);
          if (forwardDist > 0.001f) {
              forward = {forward.x / forwardDist, forward.y / forwardDist, forward.z / forwardDist};
              float pitch = -std::asin(forward.y);
              float yaw = std::atan2(forward.x, forward.z);
              activeCamera->SetRotate({pitch, yaw, 0.0f});
          }
          activeCamera->UpdateMatrix();
      }
  }

#if defined(_DEBUG) || defined(DEVELOPMENT)
  // --- 当たり判定有効無効のトグル (F4) ---
  if (engine_->GetInputManager()->IsKeyPressedDIK(0x3E /*DIK_F4*/)) {
    isCollisionEnabled_ = !isCollisionEnabled_;
    OutputDebugStringA(isCollisionEnabled_ ? "Collision: ENABLED\n"
                                           : "Collision: DISABLED\n");
  }
#endif

  // --- 当たり判定の実行 ---
  CheckAllCollisions();

  if (field_) {
    field_->Update();
  }

  // --- 建物の時間経過自動生成 ---
  if (field_ && field_->GetBuilding() && player_ && boss_) {
    Building* building = field_->GetBuilding();
    const auto& params = building->GetParams();

    // 生存している建物が上限未満のときのみタイマーを進めて生成する
    if (building->GetAliveBuildingCount() < params.maxCount) {
      buildingSpawnTimer_ += 1.0f / 60.0f;
      if (buildingSpawnTimer_ >= params.spawnInterval) {
        buildingSpawnTimer_ = 0.0f;
        building->SpawnRandomBuilding(player_->GetTranslate(), boss_->GetTargetPosition());
      }
    } else {
      buildingSpawnTimer_ = 0.0f;
    }
  }

  skydome_->Update();

  // ライトのパラメータ更新
  if (player_ && boss_) {
      dynamicArenaLight_->Update(player_->GetTranslate(), boss_->GetTargetPosition());
  }

  // =====
  // ↑ゲームの更新
  // =====

  BaseScene::Update();
  engine_->GetDrawManager()->SetEnvironmentMap(engine_->GetTextureManager()->GetWhiteCubeMapHandle());

  float screenW = static_cast<float>(engine_->GetClientWidth());
  float screenH = static_cast<float>(engine_->GetClientHeight());
  float uiScale = screenH / 720.0f;

  // 個別キーUIの更新 (1人称視点専用)
  if (player_ && player_->IsFirstPerson()) {
      InputManager* input = engine_->GetInputManager();
      bool lClick = input->IsMouseButtonDown(Mouse::Button::Left);
      bool rClick = input->IsMouseButtonDown(Mouse::Button::Right);
      bool eDown = input->IsKeyDownDIK(DIK_E);
      bool vDown = input->IsKeyDownDIK(DIK_V);
      bool spaceDown = input->IsKeyDownDIK(DIK_SPACE);

      Vector4 colorDown = {0.0f, 1.0f, 1.0f, 1.0f};
      Vector4 colorUp = {0.8f, 0.8f, 0.8f, 1.0f};

      float baseX = 460.0f;
      float baseY = screenH - 80.0f;
      float rowSpacing = -60.0f; // 上段なのでマイナス方向(Yを減らす)
      float colSpacing = 260.0f;

      // --- 上段 ---
      if (uiE_) {
          uiE_->SetUIScale(uiScale);
          uiE_->SetPositionTopLeft((baseX) * uiScale, (baseY + rowSpacing) * uiScale);
          uiE_->SetColor(eDown ? colorDown : colorUp);
          uiE_->Update();
      }
      if (uiV_) {
          uiV_->SetUIScale(uiScale);
          uiV_->SetPositionTopLeft((baseX + colSpacing) * uiScale, (baseY + rowSpacing) * uiScale);
          uiV_->SetColor(vDown ? colorDown : colorUp);
          uiV_->Update();
      }

      // --- 下段 ---
      if (uiLClickNormal_) {
          uiLClickNormal_->SetUIScale(uiScale);
          uiLClickNormal_->SetPositionTopLeft((baseX) * uiScale, (baseY) * uiScale);
          uiLClickNormal_->SetColor(lClick ? colorDown : colorUp);
          uiLClickNormal_->Update();
      }
      if (uiLClickCharged_) {
          uiLClickCharged_->SetUIScale(uiScale);
          uiLClickCharged_->SetPositionTopLeft((baseX) * uiScale, (baseY) * uiScale);
          uiLClickCharged_->SetColor(lClick ? colorDown : colorUp);
          uiLClickCharged_->Update();
      }
      if (uiRClickNormal_) {
          uiRClickNormal_->SetUIScale(uiScale);
          uiRClickNormal_->SetPositionTopLeft((baseX + colSpacing) * uiScale, (baseY) * uiScale);
          uiRClickNormal_->SetColor(rClick ? colorDown : colorUp);
          uiRClickNormal_->Update();
      }
      if (uiRClickCharged_) {
          uiRClickCharged_->SetUIScale(uiScale);
          uiRClickCharged_->SetPositionTopLeft((baseX + colSpacing) * uiScale, (baseY) * uiScale);
          uiRClickCharged_->SetColor(rClick ? colorDown : colorUp);
          uiRClickCharged_->Update();
      }
      if (uiSpace_) {
          uiSpace_->SetUIScale(uiScale);
          uiSpace_->SetPositionTopLeft((baseX + colSpacing * 2) * uiScale, (baseY) * uiScale);
          uiSpace_->SetColor(spaceDown ? colorDown : colorUp);
          uiSpace_->Update();
      }
  } else if (player_ && !player_->IsFirstPerson()) {
      InputManager* input = engine_->GetInputManager();
      bool vDown = input->IsKeyDownDIK(DIK_V);
      Vector4 colorDown = {0.0f, 1.0f, 1.0f, 1.0f};
      Vector4 colorUp = {0.8f, 0.8f, 0.8f, 1.0f};

      if (uiV_) {
          uiV_->SetUIScale(uiScale);
          uiV_->SetPositionTopLeft(20.0f * uiScale, screenH - 80.0f * uiScale);
          uiV_->SetColor(vDown ? colorDown : colorUp);
          uiV_->Update();
      }
  }

  if (cooldownWarningSprite_) {
      cooldownWarningSprite_->SetUIScale(uiScale);
      cooldownWarningSprite_->SetPositionCenter(screenW / 2.0f + 15.0f * uiScale, screenH / 2.0f + 80.0f * uiScale);
      cooldownWarningSprite_->Update();
  }



  if (keyEscSprite_) {
      keyEscSprite_->Update();
  }

  // シーン遷移
  // 爆発を続けながらホワイトアウトさせるため、専用の判定関数を使用
  if (boss_ && boss_->IsReadyForClearTransition()) {
    if (engine_ && engine_->GetSceneManager()) {
      engine_->GetSceneManager()->TransitionTo(
          "Clear", SceneTransition::Type::RadialBlurWhite, 2.5f);
    }
  }
  if (player_ && player_->IsDeathAnimationFinished()) {
    if (engine_ && engine_->GetSceneManager()) {
      engine_->GetSceneManager()->TransitionTo(
          "GameOver", SceneTransition::Type::Dissolve, 2.0f);
    }
  }
}

void GameScene::Draw() {
  if (engine_) {
      engine_->SetBlend(BlendMode::kBlendModeNormal);
      engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
      engine_->SetCull(PSOManager::CullMode::Back);
  }

  if (skydome_)
    skydome_->Draw();

  if (engine_) {
      engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
  }

  // プレイヤーが星になって吹っ飛んでいる最中は、巨大な敵やビル・地形がカメラを塞がないよう
  // 意図的に描画をスキップし、空（Skydome）だけを背景に美しく演出を見せる
  bool hideObstacles = (player_ && player_->IsBlowingAway());

  if (field_ && !hideObstacles)
    field_->Draw();
    
  if (player_)
    player_->Draw();
    
  if (boss_ && !hideObstacles)
    boss_->Draw(engine_);
    
  if (player_)
    player_->DrawParticles();

  // ★撃破演出中（ボス死亡時）はUIをすべて非表示にする
  bool isUIHidden = (boss_ && boss_->IsDead());

  // --- HPバーUI描画（スプライト：マスクやエイム） ---
  if (player_ && !isUIHidden) {
    player_->Draw2DUI(boss_.get());
  }

  // --- 3DオブジェクトとしてのUI描画（HPバー） ---
  if (player_ && !hideObstacles && !isUIHidden) {
    bool isPaused = (engine_->GetSceneManager()->GetCurrent() == "Pause");
    player_->Draw3DUI(boss_.get(), true, isPaused);
  }

  // --- 操作説明および警告スプライト描画 ---
  if (!isUIHidden) {
      if (player_ && !player_->IsFirstPerson()) {
          if (uiV_) uiV_->Draw();

          // --- クールダウン警告スプライト描画 ---
          if (player_->GetCooldownWarningTimer() > 0) {
              if ((player_->GetCooldownWarningTimer() / 10) % 2 == 0) {
                  if (cooldownWarningSprite_) cooldownWarningSprite_->Draw();
              }
          }
      } else if (player_ && player_->IsFirstPerson()) {
          // --- 1人称視点専用UI描画 ---
          if (!player_->IsKarakuriCharged()) {
              if (uiLClickNormal_) uiLClickNormal_->Draw();
              if (uiRClickNormal_) uiRClickNormal_->Draw();
          } else {
              if (uiLClickCharged_) uiLClickCharged_->Draw();
              if (uiRClickCharged_) uiRClickCharged_->Draw();
          }
          
          // 共通キーUI
          if (uiE_) uiE_->Draw();
          if (uiV_) uiV_->Draw();
          if (uiSpace_) uiSpace_->Draw();
      } 

      if (keyEscSprite_) {
          keyEscSprite_->Draw();
      }
  }
}


void GameScene::DrawDebugTab() {
#ifdef USE_IMGUI
  BaseScene::DrawDebugTab();
#endif
}



// --- 当たり判定の実装 ---

void GameScene::CheckAllCollisions() {
  if (!player_ || !boss_ || !isCollisionEnabled_)
    return;

  // 敵からプレイヤーへの攻撃
  CheckEnemyToPlayerCollisions();

  // プレイヤーから敵への攻撃
  CheckPlayerToEnemyCollisions();

  // 部位の衝突
  CheckFlyingPartsCollisions();

  // --- 建物との当たり判定 ---
  CheckPlayerBuildingCollisions();
  CheckEnemyBuildingCollisions();
  CheckFlyingPartsBuildingCollisions();
  CheckFlyingBuildingsVsEnemyCollisions();
  CheckFlyingBuildingsVsBuildingsCollisions();
}

void GameScene::CheckEnemyToPlayerCollisions() {
  if (player_->IsDead() || boss_->IsDead()) return;

  Sphere playerColliderSphere;
  playerColliderSphere.center = player_->GetCollider().center;
  playerColliderSphere.radius = player_->GetCollider().radius;

  // EnemyBeam の判定
  for (int bi = 0; bi < 3; ++bi) {
    if (boss_->GetBeam(bi) && boss_->GetBeam(bi)->IsAttackActive()) {
      if (Collision::IsOBBSphereCollision(boss_->GetBeam(bi)->GetOBB(),
                                          playerColliderSphere)) {
        if (player_->ApplyDamage(kDamageBeamToPlayer)) {
          OutputDebugStringA(
              std::format("Player Hit by Beam: {} damage\n", kDamageBeamToPlayer)
                  .c_str());
        }
      }
    }
  }

  // EnemyBomb の判定
  for (int bi = 0; bi < 3; ++bi) {
    if (boss_->GetBomb(bi) && boss_->GetBomb(bi)->IsExploding()) {
      std::vector<OBB> bombOBBs = boss_->GetBomb(bi)->GetOBBs();
      for (const auto& obb : bombOBBs) {
        if (Collision::IsOBBSphereCollision(obb, playerColliderSphere)) {
          if (player_->ApplyDamage(kDamageBombToPlayer)) {
            OutputDebugStringA(
                std::format("Player Hit by Bomb: {} damage\n", kDamageBombToPlayer)
                    .c_str());
          }
          break; // 多重ヒットを防ぐために1回の爆破で1ダメージに制限
        }
      }
    }
  }

  // EnemyStompEffects の判定
  if (boss_->GetStompEffects() && boss_->GetStompEffects()->IsActive()) {
    EnemyStompEffects *stomp = boss_->GetStompEffects();
    if (stomp->IsExplosionDamageActive() && !stomp->HasDealtExplosionDamage()) {
      Sphere explosionSphere;
      explosionSphere.center = stomp->GetBasePosition();
      explosionSphere.radius = stomp->GetExplosionRadius();
      if (Collision::IsSphereCollision(explosionSphere, playerColliderSphere)) {
        if (player_->ApplyDamage(stomp->GetExplosionDamage())) {
          stomp->SetDealtExplosionDamage(true);
        }
      }
    }
    if (stomp->IsFinalExplosionActive() && !stomp->HasDealtFinalDamage()) {
      if (Collision::IsCollision(stomp->GetFinalExplosionSphere(),
                                       playerColliderSphere)) {
        if (player_->ApplyDamage(stomp->GetFinalExplosionDamage())) {
          stomp->SetDealtFinalDamage(true);
        }
      }
    }
  }

  // EnemyTackleEffects の判定
  if (boss_->GetTackleEffects()) {
    for (auto& wave : boss_->GetTackleEffects()->GetWaves()) {
      if (!wave.hasDealtDamage) {
        if (Collision::IsOBBSphereCollision(wave.GetOBB(), playerColliderSphere)) {
          int damage = wave.isCrash ? kDamageCrashWaveToPlayer : kDamageTackleWaveToPlayer;
          if (player_->ApplyDamage(damage)) {
            wave.hasDealtDamage = true;
          }
        }
      }
    }
  }

  // 敵部位との接触判定
  auto checkHit = [&](auto *part) {
    if (!part || part->IsCompletelyDead())
      return;
    OBB partOBB = part->GetOBB();
    if (Collision::IsOBBSphereCollision(partOBB, playerColliderSphere)) {
      bool isIdle = (boss_->GetState() == EnemyState::Idle);
      bool shouldDealDamage = (!isIdle || part->IsBlownAway());

      if (shouldDealDamage && player_->ApplyDamage(kDamagePartToPlayer)) {
        // 吹き飛んでいる状態のとき、めり込んで連続ダメージになるのを防ぐため部位を反射（バウンド）させる
        if (part->IsBlownAway()) {
          Vector3 toPlayer = Math::Subtract(playerColliderSphere.center, partOBB.center);
          toPlayer.y = 0.0f;
          Vector3 normal = Math::Normalize(toPlayer);
          if (Math::Length(normal) < kMathEpsilon) normal = {0.0f, 0.0f, 1.0f};

          Vector3 vel = part->GetBlowVelocity();
          float dot = Math::Dot(vel, normal);
          if (dot > 0.0f) {
            Vector3 reflect = Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
            part->SetBlowVelocity(reflect);
          } else if (Math::Length(vel) < kMathEpsilon) {
            part->SetBlowVelocity(Math::Multiply(-2.0f, normal));
          }
        }
      }
      bool isTackle = (boss_->GetState() == EnemyState::Attack_Tackle);

      // 押し出し処理
      if (!isTackle) {
        Vector3 diff = Math::Subtract(player_->GetTranslate(), partOBB.center);
        float bestPushEval = 1e10f;
        float bestActualPushDist = 0.0f;
        Vector3 bestPushDir = {0.0f, 0.0f, 0.0f};

        for (int axis = 0; axis < 3; ++axis) {
          if (axis == 1) continue; // Y軸除外
          
          float proj = Math::Dot(diff, partOBB.orientations[axis]);
          float halfSize = (axis == 0) ? partOBB.size.x : partOBB.size.z;
          
          float actualPenetration = (halfSize + playerColliderSphere.radius) - std::abs(proj);
          float sign = (proj >= 0.0f) ? 1.0f : -1.0f;
          Vector3 pushDir = Math::Multiply(sign, partOBB.orientations[axis]);
          
          if (actualPenetration > 0.0f && actualPenetration < bestPushEval) {
              bestPushEval = actualPenetration;
              bestActualPushDist = actualPenetration;
              bestPushDir = pushDir;
          }
        }
        
        if (bestActualPushDist > 0.0f && bestPushEval < 1e9f) {
            bestPushDir.y = 0.0f;
            if (Math::Length(bestPushDir) > 0.001f) {
                bestPushDir = Math::Normalize(bestPushDir);
                Vector3 newPos = Math::Add(player_->GetTranslate(), Math::Multiply(bestActualPushDist, bestPushDir));
                player_->SetTranslate(newPos);
                playerColliderSphere.center = player_->GetCollider().center; // 更新
            }
        }
      }
    }
  };
  for (int i = 0; i < kEnemyBodyPartsCount; ++i)
    checkHit(boss_->GetBody(i));
  checkHit(boss_->GetHeadLeft());
  checkHit(boss_->GetHeadMid());
  checkHit(boss_->GetHeadRight());
}

void GameScene::CheckPlayerToEnemyCollisions() {
  const AttackCollision &attackCol = player_->GetAttackCollision();
  Sphere attackSphere;
  attackSphere.center = attackCol.center;
  attackSphere.radius = attackCol.radius;
  Vector3 playerPos = player_->GetTranslate();

  auto checkAndDamage = [&](auto *part) {
    if (!part || part->GetHP() <= 0 || part->IsBlownAway())
      return;

    // 近接攻撃
    if (attackCol.isActive &&
        Collision::IsOBBSphereCollision(part->GetOBB(), attackSphere)) {
      int damage = player_->GetDamageMelee();
      if (player_->IsKarakuriCharged()) damage = static_cast<int>(damage * player_->GetDamageMeleeChargeMultiplier());

      if (part->ApplyDamage(damage)) {
        player_->OnMeleeHit();
        if (part->GetHP() <= 0) {
          Vector3 attackDir = Math::Normalize(
              Math::Subtract(part->GetTransform().translate, playerPos));
          part->OnDestroyed(attackDir,
                            EnemyParameters::GetInstance()->GetBlowSpeed());
        }
        // Voxelの飛散方向
        Vector3 scatterDir = Math::Normalize(
            Math::Subtract(part->GetTransform().translate, playerPos));
        
        // 接触部分（剣の当たり判定）だけを飛散させるための領域(OBB)を作成
        OBB hitArea;
        hitArea.center = attackSphere.center;
        hitArea.orientations[0] = {1.0f, 0.0f, 0.0f};
        hitArea.orientations[1] = {0.0f, 1.0f, 0.0f};
        hitArea.orientations[2] = {0.0f, 0.0f, 1.0f};
        hitArea.size = {attackSphere.radius, attackSphere.radius, attackSphere.radius};

        part->ScatterAt(
            Math::Multiply(kMeleeScatterSpeedMultiplier, scatterDir),
            hitArea);
      }
    }

    if (part->GetHP() <= 0) return; // 既に破壊された場合は後続のヒット判定をスキップ

    // マシンガン
    MachineGunBullet *bullets = player_->GetMachineGunBullets();
    for (int i = 0; i < Player::GetMaxMachineGunBullets(); ++i) {
      if (!bullets[i].isActive)
        continue;
      Sphere bulletSphere = {bullets[i].position, kMachineGunBulletRadius};
      if (Collision::IsOBBSphereCollision(part->GetOBB(), bulletSphere)) {
        bullets[i].isActive = false;
        Vector3 hitPoint;
        Segment segment = { Math::Subtract(bullets[i].position, bullets[i].velocity), bullets[i].velocity };
        if (Collision::GetOBBSegmentIntersection(part->GetOBB(), segment, hitPoint)) {
          Vector3 pushDir = Math::Multiply(-1.0f, bullets[i].velocity);
          float len = Math::Length(pushDir);
          if (len > 0.001f) pushDir = Math::Normalize(pushDir);
          else pushDir = { 0.0f, 1.0f, 0.0f };
          hitPoint = Math::Add(hitPoint, Math::Multiply(0.4f, pushDir));
        } else {
          hitPoint = Collision::GetOBBSphereClosestPoint(part->GetOBB(), bulletSphere, 0.4f);
        }
        // アニメーションによるビジュアルモデルのズレ（胴体のみ）を同期補正
        if (dynamic_cast<Body*>(part)) {
          Vector3 visualOffset = Math::Subtract(part->GetDrawPosition(), part->GetOBB().center);
          hitPoint = Math::Add(hitPoint, visualOffset);
        }

        player_->PlayExplosion(hitPoint, 0.25f);
        int damage = player_->GetDamageMachineGun();
        if (player_->IsKarakuriCharged()) damage = static_cast<int>(damage * player_->GetDamageMachineGunChargeMultiplier());

        if (part->ApplyDamage(damage) && part->GetHP() <= 0) {
          part->OnDestroyed(Math::Normalize(bullets[i].velocity),
                            EnemyParameters::GetInstance()->GetBlowSpeed());
          break; // HPが0になったらループを抜けて多重破壊を防止
        }
      }
    }

    if (part->GetHP() <= 0) return; // マシンガンで破壊された場合はミサイルのヒット判定をスキップ

    // ミサイル
    MissileData *missiles = player_->GetMissiles();
    for (int i = 0; i < Player::GetMaxMissiles(); ++i) {
      if (!missiles[i].isActive)
        continue;
      Sphere missileSphere = {missiles[i].position, kMissileRadius};
      if (Collision::IsOBBSphereCollision(part->GetOBB(), missileSphere)) {
        missiles[i].isActive = false;
        Vector3 hitPoint;
        Segment segment = { Math::Subtract(missiles[i].position, missiles[i].velocity), missiles[i].velocity };
        if (Collision::GetOBBSegmentIntersection(part->GetOBB(), segment, hitPoint)) {
          Vector3 pushDir = Math::Multiply(-1.0f, missiles[i].velocity);
          float len = Math::Length(pushDir);
          if (len > 0.001f) pushDir = Math::Normalize(pushDir);
          else pushDir = { 0.0f, 1.0f, 0.0f };
          hitPoint = Math::Add(hitPoint, Math::Multiply(1.0f, pushDir));
        } else {
          hitPoint = Collision::GetOBBSphereClosestPoint(part->GetOBB(), missileSphere, 1.0f);
        }
        // アニメーションによるビジュアルモデルのズレ（胴体のみ）を同期補正
        if (dynamic_cast<Body*>(part)) {
          Vector3 visualOffset = Math::Subtract(part->GetDrawPosition(), part->GetOBB().center);
          hitPoint = Math::Add(hitPoint, visualOffset);
        }

        player_->PlayExplosion(hitPoint, 1.2f);
        int damage = player_->GetDamageMissile();
        if (player_->IsKarakuriCharged()) damage = static_cast<int>(damage * player_->GetDamageMissileChargeMultiplier());

        if (part->ApplyDamage(damage) && part->GetHP() <= 0) {
          part->OnDestroyed(Math::Normalize(missiles[i].velocity),
                            EnemyParameters::GetInstance()->GetBlowSpeed());
          break; // HPが0になったらループを抜ける
        }
      }
    }
  };

  for (int i = 0; i < kEnemyBodyPartsCount; ++i)
    checkAndDamage(boss_->GetBody(i));
  checkAndDamage(boss_->GetHeadLeft());
  checkAndDamage(boss_->GetHeadMid());
  checkAndDamage(boss_->GetHeadRight());
}

void GameScene::CheckFlyingPartsCollisions() {
  auto checkProjectile = [&](auto *projectile) {
    if (!projectile || !projectile->IsBlownAway() ||
        projectile->IsCompletelyDead())
      return;

    // 吹き飛び直後の即時衝突（自爆）を防ぐためのクールタイム
    if (projectile->GetBlowTimer() < kBlowCollisionDelay)
      return;

    OBB projectileOBB = projectile->GetOBB();

    auto checkTarget = [&](auto *target) {
      if (!target || target->GetHP() <= 0 || target->IsBlownAway())
        return;
      if (Collision::IsOBBCollision(projectileOBB, target->GetOBB())) {
        Vector3 vel = projectile->GetBlowVelocity();
        Vector3 diff = Math::Subtract(projectile->GetTransform().translate,
                                      target->GetTransform().translate);
        Vector3 normal = Math::Normalize(Vector3{diff.x, 0.0f, diff.z});
        if (Math::Length(normal) < kMathEpsilon)
          normal = kDefaultNormalZ;

        float dot = Math::Dot(vel, normal);
        if (dot < 0.0f) {
          target->ApplyDamage(kDamageProjectilePartToEnemy);
          Vector3 reflect =
              Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
          reflect.y = 0.0f;
          projectile->SetBlowVelocity(reflect);
          if (target->GetHP() <= 0) {
            target->OnDestroyed(Math::Normalize(vel),
                                EnemyParameters::GetInstance()->GetBlowSpeed());
          }
          target->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, vel),
                            target->GetOBB());
          projectile->ScatterAt(
              Math::Multiply(kCollisionScatterMultiplier, reflect),
              projectile->GetOBB());
        }
      }
    };

    auto checkBounce = [&](auto *target) {
      if (!target || (void *)target == (void *)projectile ||
          !target->IsBlownAway() || target->IsCompletelyDead())
        return;
      if (Collision::IsOBBCollision(projectileOBB, target->GetOBB())) {
        Vector3 vel1 = projectile->GetBlowVelocity();
        Vector3 vel2 = target->GetBlowVelocity();
        Vector3 diff = Math::Subtract(projectile->GetTransform().translate,
                                      target->GetTransform().translate);
        Vector3 normal = Math::Normalize(Vector3{diff.x, 0.0f, diff.z});
        if (Math::Length(normal) < kMathEpsilon)
          normal = kDefaultNormalZ;

        Vector3 relVel = Math::Subtract(vel1, vel2);
        float dot = Math::Dot(relVel, normal);
        if (dot < 0.0f) {
          Vector3 change = Math::Multiply(dot, normal);
          Vector3 b1 = Math::Subtract(vel1, change);
          Vector3 b2 = Math::Add(vel2, change);
          b1.y = b2.y = 0.0f;
          projectile->SetBlowVelocity(b1);
          target->SetBlowVelocity(b2);

          // 押し戻し処理（めり込み・反発ループ防止）
          Vector3 pPos = projectile->GetTransform().translate;
          Vector3 tPos = target->GetTransform().translate;
          float dist = Math::Length(diff); // diffは pPos - tPos、Y無理版
          float idealDist = 4.0f; // 部位のサイズからの概算最小距離
          if (dist < idealDist) {
              float pushOut = (idealDist - dist) * 0.5f;
              projectile->SetPosition(Math::Add(pPos, Math::Multiply(pushOut, normal)));
              target->SetPosition(Math::Subtract(tPos, Math::Multiply(pushOut, normal)));
          }

          /// @brief 衝突範囲の中間点に局所的な破片飛散用OBBを生成する
          OBB impactOBB;
          impactOBB.center = Math::Multiply(0.5f, Math::Add(projectileOBB.center, target->GetOBB().center));
          impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f};
          impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f};
          impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
          impactOBB.size = {3.0f, 3.0f, 3.0f};

          projectile->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, b1), impactOBB);
          target->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, b2), impactOBB);
        }
      }
    };

    for (int i = 0; i < kEnemyBodyPartsCount; ++i) {
      checkTarget(boss_->GetBody(i));
      checkBounce(boss_->GetBody(i));
    }
    checkTarget(boss_->GetHeadLeft());
    checkBounce(boss_->GetHeadLeft());
    checkTarget(boss_->GetHeadMid());
    checkBounce(boss_->GetHeadMid());
    checkTarget(boss_->GetHeadRight());
    checkBounce(boss_->GetHeadRight());
  };

  for (int i = 0; i < kEnemyBodyPartsCount; ++i)
    checkProjectile(boss_->GetBody(i));
  checkProjectile(boss_->GetHeadLeft());
  checkProjectile(boss_->GetHeadMid());
  checkProjectile(boss_->GetHeadRight());
}

// =============================================
// --- 建物との当たり判定 ---
// =============================================

void GameScene::CheckPlayerBuildingCollisions() {
  Building *building = field_ ? field_->GetBuilding() : nullptr;
  if (!building || !player_)
    return;

  // 1. プレイヤーの押し戻し
  Vector3 playerPos = player_->GetTranslate();
  float playerRadius = player_->GetCollider().radius;

  for (int i = 0; i < building->GetBuildingCount(); ++i) {
    if (!building->IsBuildingAlive(i))
      continue;

    OBB buildingOBB = building->GetBuildingOBB(i);
    Sphere playerSphere;
    playerSphere.center = playerPos;
    playerSphere.radius = playerRadius;

    if (Collision::IsOBBSphereCollision(buildingOBB, playerSphere)) {
      // 押し戻し：最も浅い軸方向に押し出す
      Vector3 diff = Math::Subtract(playerPos, buildingOBB.center);
      float bestPushDist = 1e10f;
      Vector3 bestPushDir = {0.0f, 0.0f, 0.0f};

      for (int axis = 0; axis < 3; ++axis) {
        if (axis == 1)
          continue; // Y軸は押し戻さない

        float proj = Math::Dot(diff, buildingOBB.orientations[axis]);
        float halfSize = 0.0f;
        if (axis == 0)
          halfSize = buildingOBB.size.x;
        else if (axis == 2)
          halfSize = buildingOBB.size.z;

        float penetration = (halfSize + playerRadius) - std::abs(proj);
        if (penetration > 0.0f && penetration < bestPushDist) {
          bestPushDist = penetration;
          float sign = (proj >= 0.0f) ? 1.0f : -1.0f;
          bestPushDir = Math::Multiply(sign, buildingOBB.orientations[axis]);
        }
      }

      if (bestPushDist < 1e10f) {
        playerPos =
            Math::Add(playerPos, Math::Multiply(bestPushDist, bestPushDir));
      }
    }
  }

  // 押し戻し後の座標をプレイヤーに反映
  player_->SetTranslate(playerPos);

  // 2. プレイヤー攻撃 → 建物
  const AttackCollision &attackCol = player_->GetAttackCollision();
  Sphere attackSphere;
  attackSphere.center = attackCol.center;
  attackSphere.radius = attackCol.radius;

  for (int i = 0; i < building->GetBuildingCount(); ++i) {
    if (!building->IsBuildingAlive(i))
      continue;
    OBB bOBB = building->GetBuildingOBB(i);

    // 近接攻撃
    if (attackCol.isActive &&
        Collision::IsOBBSphereCollision(bOBB, attackSphere)) {
      Vector3 attackDir =
          Math::Normalize(Math::Subtract(bOBB.center, playerPos));
      building->ApplyDamage(i, kDamageMeleeToBuilding, attackDir, 0.5f);
      
      OBB impactOBB;
      impactOBB.center = attackSphere.center;
      impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f};
      impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f};
      impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
      
      // ビルのボクセルはY軸方向に大きく引き伸ばされており（最大130mなどを16分割）、
      // 剣の判定（半径1.5m）だとボクセルとボクセルの隙間をすり抜けてしまうため、
      // 確実に火花が飛ぶように判定を縦に少し広げる。全体が爆散しないよう8倍から3倍へ縮小
      impactOBB.size = {attackSphere.radius * 2.0f, attackSphere.radius * 3.0f, attackSphere.radius * 2.0f};
      building->ScatterAt(i, Math::Multiply(3.0f, attackDir), impactOBB);
    }

    // マシンガン
    MachineGunBullet *bullets = player_->GetMachineGunBullets();
    for (int b = 0; b < Player::GetMaxMachineGunBullets(); ++b) {
      if (!bullets[b].isActive)
        continue;
      Sphere bulletSphere = {bullets[b].position, kMachineGunBulletRadius};
      if (Collision::IsOBBSphereCollision(bOBB, bulletSphere)) {
        bullets[b].isActive = false;
        Vector3 attackDir = Math::Normalize(bullets[b].velocity);
        building->ApplyDamage(i, kDamageMachineGunToBuilding, attackDir, 0.3f);
      }
    }

    // ミサイル
    MissileData *missiles = player_->GetMissiles();
    for (int m = 0; m < Player::GetMaxMissiles(); ++m) {
      if (!missiles[m].isActive)
        continue;
      Sphere missileSphere = {missiles[m].position, kMissileRadius};
      if (Collision::IsOBBSphereCollision(bOBB, missileSphere)) {
        missiles[m].isActive = false;
        Vector3 attackDir = Math::Normalize(missiles[m].velocity);
        building->ApplyDamage(i, kDamageMissileToBuilding, attackDir, 0.8f);
      }
    }
  }
}

void GameScene::CheckEnemyBuildingCollisions() {
  Building *building = field_ ? field_->GetBuilding() : nullptr;
  if (!building || !boss_)
    return;

  // ボス全体が死亡している場合は、本体OBBやビーム等での建物破壊判定をスキップ
  // （吹き飛んだ部位による演出上の破壊は CheckFlyingPartsBuildingCollisions 等で行う）
  if (boss_->IsDead()) return;

  for (int i = 0; i < building->GetBuildingCount(); ++i) {
    if (!building->IsBuildingAlive(i))
      continue;
    OBB bOBB = building->GetBuildingOBB(i);

    // 敵本体OBB → 建物（その場で爆散）
    OBB enemyOBB = boss_->GetOBB();
    if (Collision::IsOBBCollision(enemyOBB, bOBB)) {
      building->MarkDestroyed(i);
      continue;
    }

    // ビーム → 建物（その場で爆散）
    bool buildingDestroyedByBeam = false;
    for (int bi = 0; bi < 3; ++bi) {
      if (boss_->GetBeam(bi) && boss_->GetBeam(bi)->IsAttackActive()) {
        if (Collision::IsOBBCollision(boss_->GetBeam(bi)->GetOBB(), bOBB)) {
          building->MarkDestroyed(i);
          buildingDestroyedByBeam = true;
          break;
        }
      }
    }
    if (buildingDestroyedByBeam) continue;

    // ボム → 建物（その場で爆散）
    bool buildingDestroyedByBomb = false;
    for (int bi = 0; bi < 3; ++bi) {
      if (boss_->GetBomb(bi) && boss_->GetBomb(bi)->IsExploding()) {
        std::vector<OBB> bombOBBs = boss_->GetBomb(bi)->GetOBBs();
        for (const auto& obb : bombOBBs) {
          if (Collision::IsOBBCollision(obb, bOBB)) {
            building->MarkDestroyed(i);
            buildingDestroyedByBomb = true;
            break;
          }
        }
        if (buildingDestroyedByBomb) break;
      }
    }
    if (buildingDestroyedByBomb) continue;

    // スタンプ → 建物（その場で爆散）
    if (boss_->GetStompEffects() && boss_->GetStompEffects()->IsActive()) {
      EnemyStompEffects *stomp = boss_->GetStompEffects();
      if (stomp->IsExplosionDamageActive()) {
        Sphere explosionSphere;
        explosionSphere.center = stomp->GetBasePosition();
        explosionSphere.radius = stomp->GetExplosionRadius();
        if (Collision::IsOBBSphereCollision(bOBB, explosionSphere)) {
          building->MarkDestroyed(i);
          continue;
        }
      }
      if (stomp->IsFinalExplosionActive()) {
        if (Collision::IsCollision(bOBB, stomp->GetFinalExplosionSphere())) {
          building->MarkDestroyed(i);
          continue;
        }
      }
    }

    // Bite（Phase2の首の突進）→ 建物（その場で爆散）
    if (boss_->GetIsPhase2()) {
      auto checkBiteHead = [&](auto *head) {
        if (!head || head->IsCompletelyDead() || head->IsBlownAway())
          return;
        if (Collision::IsOBBCollision(head->GetOBB(), bOBB)) {
          building->MarkDestroyed(i);
        }
      };
      checkBiteHead(boss_->GetHeadLeft());
      if (building->IsBuildingDestroyed(i))
        continue;
      checkBiteHead(boss_->GetHeadMid());
      if (building->IsBuildingDestroyed(i))
        continue;
      checkBiteHead(boss_->GetHeadRight());
    }
  }
}

void GameScene::CheckFlyingPartsBuildingCollisions() {
  Building *building = field_ ? field_->GetBuilding() : nullptr;
  if (!building || !boss_)
    return;

  auto checkPartVsBuildings = [&](auto *part) {
    if (!part || !part->IsBlownAway() || part->IsCompletelyDead())
      return;
    OBB partOBB = part->GetOBB();

    for (int i = 0; i < building->GetBuildingCount(); ++i) {
      if (!building->IsBuildingAlive(i))
        continue;
      OBB bOBB = building->GetBuildingOBB(i);

      if (Collision::IsOBBCollision(partOBB, bOBB)) {
        // 建物にダメージ
        Vector3 vel = part->GetBlowVelocity();
        Vector3 dir = Math::Normalize(vel);
        building->ApplyDamage(i, kDamagePartToBuilding, dir, 0.6f);
        
        /// @brief 衝突範囲の中間点に局所的な破片飛散用OBBを生成する
        OBB impactOBB;
        impactOBB.center = Math::Multiply(0.5f, Math::Add(partOBB.center, bOBB.center));
        impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f};
        impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f};
        impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
        impactOBB.size = {3.0f, 3.0f, 3.0f};
        
        building->ScatterAt(i, Math::Multiply(kCollisionScatterMultiplier, vel), impactOBB);

        // 部位の反射
        Vector3 diff = Math::Subtract(partOBB.center, bOBB.center);
        Vector3 normal = Math::Normalize(Vector3{diff.x, 0.0f, diff.z});
        if (Math::Length(normal) < kMathEpsilon)
          normal = {0.0f, 0.0f, 1.0f};

        float dot = Math::Dot(vel, normal);
        if (dot < 0.0f) {
          Vector3 reflect =
              Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
          reflect.y = 0.0f;
          part->SetBlowVelocity(reflect);
        }
      }
    }
  };

  for (int i = 0; i < kEnemyBodyPartsCount; ++i) {
    checkPartVsBuildings(boss_->GetBody(i));
  }
  checkPartVsBuildings(boss_->GetHeadLeft());
  checkPartVsBuildings(boss_->GetHeadMid());
  checkPartVsBuildings(boss_->GetHeadRight());
}

void GameScene::CheckFlyingBuildingsVsEnemyCollisions() {
  Building *building = field_ ? field_->GetBuilding() : nullptr;
  if (!building || !boss_)
    return;

  auto checkPartVsFlyingBuilding = [&](auto *part, int buildingIdx) {
    if (!part || part->GetHP() <= 0 || part->IsBlownAway())
      return;
    OBB partOBB = part->GetOBB();
    OBB bOBB = building->GetBuildingOBB(buildingIdx);

    if (Collision::IsOBBCollision(partOBB, bOBB)) {
      // 敵部位にダメージ
      if (part->ApplyDamage(kDamageFlyingBuildingToEnemy)) {
        if (part->GetHP() <= 0) {
          Vector3 vel = building->GetBlowVelocity(buildingIdx);
          part->OnDestroyed(Math::Normalize(vel),
                            EnemyParameters::GetInstance()->GetBlowSpeed());
        }
        // 部位にパーティクル散らし
        Vector3 vel = building->GetBlowVelocity(buildingIdx);
        
        /// @brief 衝突範囲の中間点に局所的な破片飛散用OBBを生成する
        OBB impactOBB;
        impactOBB.center = Math::Multiply(0.5f, Math::Add(partOBB.center, bOBB.center));
        impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f};
        impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f};
        impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
        impactOBB.size = {3.0f, 3.0f, 3.0f};
        
        part->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, vel), impactOBB);
      }
      // 飛んだ建物は部位に当たったので削れる
      Vector3 velForScatter = building->GetBlowVelocity(buildingIdx);
      
      /// @brief 衝突範囲の中間点に局所的な破片飛散用OBBを生成する
      OBB impactOBB2;
      impactOBB2.center = Math::Multiply(0.5f, Math::Add(partOBB.center, bOBB.center));
      impactOBB2.orientations[0] = {1.0f, 0.0f, 0.0f};
      impactOBB2.orientations[1] = {0.0f, 1.0f, 0.0f};
      impactOBB2.orientations[2] = {0.0f, 0.0f, 1.0f};
      impactOBB2.size = {3.0f, 3.0f, 3.0f};
      
      building->ScatterAt(buildingIdx, Math::Multiply(kCollisionScatterMultiplier, velForScatter), impactOBB2);
      
      // 飛んだ建物は爆散（即消滅）
      building->MarkDestroyed(buildingIdx);
    }
  };

  for (int bi = 0; bi < building->GetBuildingCount(); ++bi) {
    if (!building->IsBuildingBlownAway(bi))
      continue;

    for (int i = 0; i < kEnemyBodyPartsCount; ++i) {
      if (building->IsBuildingDestroyed(bi))
        break;
      checkPartVsFlyingBuilding(boss_->GetBody(i), bi);
    }
    if (building->IsBuildingDestroyed(bi))
      continue;
    checkPartVsFlyingBuilding(boss_->GetHeadLeft(), bi);
    if (building->IsBuildingDestroyed(bi))
      continue;
    checkPartVsFlyingBuilding(boss_->GetHeadMid(), bi);
    if (building->IsBuildingDestroyed(bi))
      continue;
    checkPartVsFlyingBuilding(boss_->GetHeadRight(), bi);
  }
}

void GameScene::CheckFlyingBuildingsVsBuildingsCollisions() {
  Building *building = field_ ? field_->GetBuilding() : nullptr;
  if (!building)
    return;

  for (int fi = 0; fi < building->GetBuildingCount(); ++fi) {
    if (!building->IsBuildingBlownAway(fi))
      continue;

    OBB flyingOBB = building->GetBuildingOBB(fi);
    Vector3 flyingVel = building->GetBlowVelocity(fi);

    for (int ti = 0; ti < building->GetBuildingCount(); ++ti) {
      if (fi == ti)
        continue;
      if (!building->IsBuildingAlive(ti))
        continue; // 生きている建物のみ対象

      OBB targetOBB = building->GetBuildingOBB(ti);

      if (Collision::IsOBBCollision(flyingOBB, targetOBB)) {
        // 建物にダメージ
        Vector3 dir = Math::Normalize(flyingVel);
        building->ApplyDamage(ti, kDamageFlyingBuildingToBuilding, dir, 0.4f);

        // 各ビルにパーティクル散らし
        /// @brief 衝突範囲の中間点に局所的な破片飛散用OBBを生成する
        OBB impactOBB;
        impactOBB.center = Math::Multiply(0.5f, Math::Add(flyingOBB.center, targetOBB.center));
        impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f};
        impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f};
        impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
        impactOBB.size = {3.0f, 3.0f, 3.0f};
        
        building->ScatterAt(ti, Math::Multiply(kCollisionScatterMultiplier, flyingVel), impactOBB);
        building->ScatterAt(fi, Math::Multiply(kCollisionScatterMultiplier, Math::Multiply(-1.0f, flyingVel)), impactOBB);

        // 飛んだ建物は反射
        Vector3 diff = Math::Subtract(flyingOBB.center, targetOBB.center);
        Vector3 normal = Math::Normalize(Vector3{diff.x, 0.0f, diff.z});
        if (Math::Length(normal) < kMathEpsilon)
          normal = {0.0f, 0.0f, 1.0f};

        float dot = Math::Dot(flyingVel, normal);
        if (dot < 0.0f) {
          Vector3 reflect =
              Math::Subtract(flyingVel, Math::Multiply(2.0f * dot, normal));
          reflect.y = 0.0f;
          building->SetBlowVelocity(fi, reflect);
          flyingVel = reflect; // 以降のチェックでも反射後の速度を使う
        }
      }
    }
  }
}

bool GameScene::IsWhiteoutContext() const {
    return boss_ && boss_->IsDead();
}
