#include "GameScene.h"

#include "Framework/SceneManager.h"
#include "Irufemi.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/AreaLight.h"

#include "actors/player/Player.h" 
#include "actors/enemy/Enemy.h"
#include "actors/enemy/EnemyParameters.h"
#include "contents/field/Field.h"
#include "contents/skydome/Skydome.h"

#include "Engine/Core/Math/Geometry/Collision.h"

#include <Windows.h>

GameScene::GameScene() {}

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

    // ★マウスはエンジン(InputManager)が管理している
    // GameScene での初期化・保持は不要になりました

    // プレイヤーの初期化（エンジン側のポインタのみ渡す）
    player_ = std::make_unique<Player>();
    player_->Initialize(engine_->GetInputManager(), camera_.get(), engine_);

    boss_ = std::make_unique<Enemy>();
    boss_->Initialize(camera_.get(), engine_);

    field_ = std::make_unique<Field>(camera_.get(), engine_);
    field_->Initialize();

    skydome_ = std::make_unique<Skydome>();
    skydome_->Initialize(camera_.get());

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLight_->direction = { 0.0f, -1.0f, 1.0f };
    directionalLight_->intensity = 1.0f;
}

// 更新
void GameScene::Update() {

#ifdef USE_IMGUI
  ImGui::Begin("Debug");
  ImGui::Checkbox("Debug Camera", &debugMode_);
  ImGui::End();
#endif

  // =====
  // ↓ゲームの更新
  // =====

    // 独自のマウス更新は二重更新（デルタ値の消失）の原因になるため削除

  // プレイヤーの更新
  if (player_ && !debugMode_) {
    // ★プレイヤーにボスの座標を毎フレーム教える（機関銃とミサイルのオートエイム用）
    if (boss_) {
      player_->SetTargetPosition(boss_->GetGlobalTransform().translate);
    }
    player_->Update();
  }

  if (boss_) {
    boss_->Update(player_.get());
  }

  // --- 当たり判定の実装 ---
  if (player_ && boss_) {
    // Playerの攻撃判定（近接）
    const AttackCollision &attackCol = player_->GetAttackCollision();
    Sphere attackSphere;
    attackSphere.center = attackCol.center;
    attackSphere.radius = attackCol.radius;
    Vector3 playerPos = player_->GetTranslate(); // 攻撃方向計算用

    // Player vs EnemyBeam の判定
    if (boss_->GetBeam() && boss_->GetBeam()->IsActive() &&
        boss_->IsFiringRealBeam()) {
      Sphere playerColliderSphere;
      playerColliderSphere.center = player_->GetCollider().center;
      playerColliderSphere.radius = player_->GetCollider().radius;

      if (Collision::IsOBBSphereCollision(boss_->GetBeam()->GetOBB(),
                                          playerColliderSphere)) {
        // ビームに当たった場合プレイヤーにダメージを与える
        player_->ApplyDamage(10);
      }
    }

    auto checkAndDamage = [&](auto *part) {
      if (!part || part->GetHP() <= 0 || part->IsBlownAway())
        return;

      // 1. 近接攻撃の判定
      if (attackCol.isActive &&
          Collision::IsOBBSphereCollision(part->GetOBB(), attackSphere)) {
        part->ApplyDamage(100); // 仮のダメージ量
        if (part->GetHP() <= 0) {
          // 攻撃方向：プレイヤーから対象部位へのベクトル
          Vector3 attackDir = Math::Normalize(
              Math::Subtract(part->GetTransform().translate, playerPos));
          part->OnDestroyed(attackDir,
                            EnemyParameters::GetInstance()->GetBlowSpeed());
        }

        // 衝突エフェクト（近接攻撃）
        OBB attackOBB;
        attackOBB.center = attackSphere.center;
        attackOBB.orientations[0] = {1, 0, 0};
        attackOBB.orientations[1] = {0, 1, 0};
        attackOBB.orientations[2] = {0, 0, 1};
        attackOBB.size = {attackCol.radius * 1.5f, attackCol.radius * 1.5f, attackCol.radius * 1.5f};
        
        // 攻撃方向を考慮した初速ではじけさせる
        Vector3 attackDir = Math::Normalize(
            Math::Subtract(part->GetTransform().translate, playerPos));
        part->ScatterAt(Math::Multiply(1.0f, attackDir), attackOBB);
      }

      // 2. マシンガンの弾の判定
      MachineGunBullet *bullets = player_->GetMachineGunBullets();
      for (int j = 0; j < Player::GetMaxMachineGunBullets(); ++j) {
        if (!bullets[j].isActive)
          continue;
        Sphere bulletSphere;
        bulletSphere.center = bullets[j].position;
        bulletSphere.radius = 1.0f; // 余裕を持たせた半径
        if (Collision::IsOBBSphereCollision(part->GetOBB(), bulletSphere)) {
          bullets[j].isActive = false;     // 弾丸消滅
          part->ApplyDamage(10); // マシンガンのダメージ
          if (part->GetHP() <= 0) {
            Vector3 attackDir = Math::Normalize(bullets[j].velocity);
            part->OnDestroyed(attackDir,
                              EnemyParameters::GetInstance()->GetBlowSpeed());
          }
        }
      }

      // 3. ミサイルの判定
      MissileData *missiles = player_->GetMissiles();
      for (int k = 0; k < Player::GetMaxMissiles(); ++k) {
        if (!missiles[k].isActive)
          continue;
        Sphere missileSphere;
        missileSphere.center = missiles[k].position;
        missileSphere.radius = 2.0f; // ミサイルの当たり判定を大きめに
        if (Collision::IsOBBSphereCollision(part->GetOBB(), missileSphere)) {
          missiles[k].isActive = false;    // ミサイル消滅
          part->ApplyDamage(50); // ミサイルのダメージ
          if (part->GetHP() <= 0) {
            Vector3 attackDir = Math::Normalize(missiles[k].velocity);
            part->OnDestroyed(attackDir,
                              EnemyParameters::GetInstance()->GetBlowSpeed());
          }
        }
      }
    };

    // 各部位に対して判定
    for (int i = 0; i < 3; ++i) {
      checkAndDamage(boss_->GetBody(i));
    }
    checkAndDamage(boss_->GetHeadLeft());
    checkAndDamage(boss_->GetHeadMid());
    checkAndDamage(boss_->GetHeadRight());
    // 吹き飛んだ部位(projectile) vs 生存している各部位(target) の判定
    auto checkProjectileHitPart = [&](auto *projectile) {
      if (!projectile || !projectile->IsBlownAway() ||
          projectile->IsCompletelyDead())
        return;

      // 毎回のターゲット（全部位）との判定ループ内で何度もGetOBB()を呼ばないようにキャッシュする
      OBB projectileOBB = projectile->GetOBB();

      auto checkTarget = [&](auto *target) {
        // targetはまだ生きているか（吹き飛んでいないか）
        if (!target || target->GetHP() <= 0 || target->IsBlownAway())
          return;

        // OBB同士の判定(部位 vs 部位)
        // projectile->GetOBB() は親ループで1度だけ取得・計算するようにキャッシュ
        if (Collision::IsOBBCollision(projectileOBB, target->GetOBB())) {
          Vector3 vel = projectile->GetBlowVelocity();
          Vector3 diff = Math::Subtract(projectile->GetTransform().translate,
                                        target->GetTransform().translate);

          // XZ平面での法線を計算
          Vector3 normal = {diff.x, 0.0f, diff.z};
          float normalLen = Math::Length(normal);
          if (normalLen > 0.001f) {
            normal = {normal.x / normalLen, 0.0f, normal.z / normalLen};
          } else {
            normal = {0.0f, 0.0f, 1.0f};
          }

          // 部位の速度と、ターゲットから部位への方向ベクトルの内積
          float dot = Math::Dot(vel, normal);

          // dot < 0.0f
          // なら、部位がターゲットに向かって飛んできている（めり込み・連続ヒット防止）
          if (dot < 0.0f) {
            // 当たった部位のみにダメージを与える
            target->ApplyDamage(500);

            // 反射ベクトル: R = V - 2(V・N)N
            Vector3 reflect =
                Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
            reflect.y = 0.0f; // Y方向には飛ばないように固定

            // 速度を反転・反射させる
            projectile->SetBlowVelocity(reflect);

            // ターゲットのHPが尽きた場合
            if (target->GetHP() <= 0) {
              float velLen = Math::Length(vel);
              Vector3 attackDir = {0.0f, 0.0f, 1.0f};
              if (velLen > 0.001f) {
                attackDir = {vel.x / velLen, 0.0f, vel.z / velLen};
              }
              target->OnDestroyed(
                  attackDir, EnemyParameters::GetInstance()->GetBlowSpeed());
            }

            // 衝突エフェクト（部位同士の衝突）
            // ターゲットのOBBをそのままはじける領域として指定
            target->ScatterAt(Math::Multiply(-0.5f, vel), target->GetOBB());
            projectile->ScatterAt(Math::Multiply(-0.5f, reflect), projectile->GetOBB());
          }
        }
      };

      for (int i = 0; i < 3; ++i)
        checkTarget(boss_->GetBody(i));
      checkTarget(boss_->GetHeadLeft());
      checkTarget(boss_->GetHeadMid());
      checkTarget(boss_->GetHeadRight());
    };

    for (int i = 0; i < 3; ++i)
      checkProjectileHitPart(boss_->GetBody(i));
    checkProjectileHitPart(boss_->GetHeadLeft());
    checkProjectileHitPart(boss_->GetHeadMid());
    checkProjectileHitPart(boss_->GetHeadRight());
  }

  if (field_) {
    field_->Update();
  }

  skydome_->Update();

  // =====
  // ↑ゲームの更新
  // =====

  // --- カメラの更新 ---
  if (debugMode_) {
    // デバッグカメラを更新
    debugCamera_->Update();
    // デバッグカメラの計算結果をメインカメラに上書きする
    const Camera &dbgCam = debugCamera_->GetCamera();
    camera_->SetViewMatrix(dbgCam.GetViewMatrix());
    camera_->SetTranslate(dbgCam.GetTranslate());
    camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
  } else {
      camera_->Debug("Main Camera");
      // 通常カメラの更新（プレイヤーのカメラ位置を反映する）
    camera_->Update();
  }

  // --- フレーム共通データのセット ---
  CameraForGPU cameraForGpu;
  cameraForGpu.view = camera_->GetViewMatrix();
  cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
  cameraForGpu.worldPosition = camera_->GetTranslate();

  std::vector<PointLight *> pLights;
  std::vector<SpotLight *> sLights;
  std::vector<AreaLight *> aLights;

  engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_,
                                          pLights, sLights, aLights);

  // enemyが死んだときクリアシーンに遷移する
  if (boss_ && boss_->IsDead()) {
    if (engine_ && engine_->GetSceneManager()) {
      engine_->GetSceneManager()->Request("Clear");
    }
  }
}

// 描画
void GameScene::Draw() {

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);
    engine_->ApplyPSO();

    skydome_->Draw();

    if (field_) {
        field_->Draw();
    }

    if (player_) {
        player_->Draw();
    }

    if (boss_) {
        boss_->Draw(engine_);
    }

    // --- 透明オブジェクト（パーティクルなど）を最後に描画 ---
    if (player_) {
        player_->DrawParticles();
    }
}

void GameScene::PauseUpdate() {}
void GameScene::PauseDraw() {}
