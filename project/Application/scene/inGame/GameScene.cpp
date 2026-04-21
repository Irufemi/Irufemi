#include "GameScene.h"
#include <algorithm>
#include <format>

#include "Framework/SceneManager.h"
#include "Irufemi.h"

#include "Graphics/Data/AreaLight.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"

#include "Graphics/PostProcess/PostProcessManager.h"
#include "actors/enemy/Enemy.h"
#include "actors/enemy/EnemyParameters.h"
#include "actors/player/Player.h"
#include "contents/field/Field.h"
#include "contents/field/building/building.h"
#include "contents/skydome/Skydome.h"

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

GameScene::GameScene() {}

GameScene::~GameScene() {}

void GameScene::Initialize(IrufemiEngine *engine) {
  engine_ = engine;

  camera_ = std::make_unique<Camera>();
  camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
  camera_->SetTranslate(kDefaultCameraPos);
  camera_->UpdateMatrix();

  debugCamera_ = std::make_unique<DebugCamera>();
  debugCamera_->Initialize(engine_->GetInputManager(),
                           engine_->GetClientWidth(),
                           engine_->GetClientHeight());
  debugMode_ = false;

  // プレイヤーの初期化
  player_ = std::make_unique<Player>();
  player_->Initialize(engine_->GetInputManager(), camera_.get(), engine_);

  boss_ = std::make_unique<Enemy>();
  boss_->Initialize(camera_.get(), engine_);

  field_ = std::make_unique<Field>(camera_.get(), engine_);
  field_->Initialize();

  skydome_ = std::make_unique<Skydome>();
  skydome_->Initialize(camera_.get());

  directionalLight_ = std::make_unique<DirectionalLight>();
  directionalLight_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLight_->direction = kDefaultLightDir;
  directionalLight_->intensity = 1.0f;

  auto pLight = std::make_unique<PointLight>();
  pLight->color = {1.0f, 0.9f, 0.8f, 1.0f}; // やや暖色寄りの白
  pLight->intensity = 5.0f;
  pLight->radius = 30.0f;
  pLight->decay = 1.0f;
  pLight->isActive = 1;
  pointLights_.push_back(std::move(pLight));
}

void GameScene::Update() {
  // =====
  // ↓ゲームの更新
  // =====

  // プレイヤーの更新
  if (player_ && !debugMode_) {
    // ボスの座標を毎フレーム教える
    if (boss_) {
      Vector3 bossPos = boss_->GetGlobalTransform().translate;
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
    boss_->Update(player_.get());
  }

  // --- 当たり判定有効無効のトグル (F4) ---
  if (engine_->GetInputManager()->IsKeyPressedDIK(0x3E /*DIK_F4*/)) {
    isCollisionEnabled_ = !isCollisionEnabled_;
    OutputDebugStringA(isCollisionEnabled_ ? "Collision: ENABLED\n"
                                           : "Collision: DISABLED\n");
  }

  // --- 当たり判定の実行 ---
  CheckAllCollisions();

  if (field_) {
    field_->Update();
  }

  skydome_->Update();

  // ライトのパラメータ更新
  UpdateDynamicLights();

  // =====
  // ↑ゲームの更新
  // =====

  // カメラとフレームデータの更新
  UpdateCameraAndFrameData();

  // シーン遷移
  if (boss_ && boss_->IsDead()) {
    if (engine_ && engine_->GetSceneManager()) {
      engine_->GetSceneManager()->TransitionTo(
          "Clear", SceneTransition::Type::RadialBlur, 1.5f);
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
  // --- 1. シャドウパス ---
  engine_->GetDrawManager()->BeginShadowPass();
  if (player_)
    player_->Draw();
  if (boss_)
    boss_->Draw(engine_);
  engine_->GetDrawManager()->EndShadowPass();

  // --- 2. メインパス ---
  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
  engine_->SetCull(PSOManager::CullMode::Back);
  engine_->ApplyPSO();

  skydome_->Draw();
  if (field_)
    field_->Draw();
  if (player_)
    player_->Draw();
  if (boss_)
    boss_->Draw(engine_);
  if (player_)
    player_->DrawParticles();
}

void GameScene::PauseUpdate() { UpdateCameraAndFrameData(); }

void GameScene::PauseDraw() {}

void GameScene::DrawDebugTab() {
#ifdef USE_IMGUI
  if (camera_) {
    if (ImGui::BeginTabItem("Main Camera")) {
      ImGui::Checkbox("Debug Camera Mode", &debugMode_);
      if (debugMode_ && debugCamera_) {
        if (ImGui::Button("Top-Down"))
          debugCamera_->SetPreset(DebugCamera::Preset::TopDown, *camera_);
        ImGui::SameLine();
        if (ImGui::Button("Diagonal"))
          debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *camera_);
        ImGui::SameLine();
        if (ImGui::Button("Front"))
          debugCamera_->SetPreset(DebugCamera::Preset::Front, *camera_);
        ImGui::SameLine();
        if (ImGui::Button("Snap to Current"))
          debugCamera_->SetPreset(DebugCamera::Preset::Current, *camera_);

        ImGui::Separator();
        ImGui::Text("Debug Camera Controls");
        debugCamera_->GetCamera().DrawDebugContents();
        float dist = debugCamera_->GetDistance();
        if (ImGui::DragFloat("Orbit Distance", &dist, kDebugCameraDragSpeed,
                             kDebugCameraDistMin, kDebugCameraDistMax)) {
          debugCamera_->SetDistance(dist);
        }
      } else {
        camera_->DrawDebugContents();
      }
      ImGui::EndTabItem();
    }
  }
  DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_,
                       areaLights_);
  if (ImGui::BeginTabItem("InGame")) {
    ImGui::Checkbox("Debug Camera", &debugMode_);
    ImGui::EndTabItem();
  }
#endif
}

void GameScene::UpdateCameraAndFrameData() {
  if (PressedDIK(kKeyDebugCameraToggle)) {
    debugMode_ = !debugMode_;
    if (debugMode_) {
      if (isFirstDebug_) {
        debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *camera_);
        isFirstDebug_ = false;
      }
    } else {
      if (player_)
        player_->Update();
      camera_->Update();
    }
  }

  if (debugMode_) {
    debugCamera_->Update();
    const Camera &dbgCam = debugCamera_->GetCamera();
    camera_->SetViewMatrix(dbgCam.GetViewMatrix());
    camera_->SetTranslate(dbgCam.GetTranslate());
    camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
  } else {
    camera_->Update();
  }

  CameraForGPU cameraForGpu;
  cameraForGpu.view = camera_->GetViewMatrix();
  cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
  cameraForGpu.worldPosition = camera_->GetTranslate();

  std::vector<PointLight *> pLights;
  for (auto &pl : pointLights_)
    pLights.push_back(pl.get());
  std::vector<SpotLight *> sLights;
  for (auto &sl : spotLights_)
    sLights.push_back(sl.get());
  std::vector<AreaLight *> aLights;
  for (auto &al : areaLights_)
    aLights.push_back(al.get());

  engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_,
                                          pLights, sLights, aLights);
  engine_->GetDrawManager()->SetEnvironmentMap(
      engine_->GetTextureManager()->GetWhiteCubeMapHandle());
}

void GameScene::UpdateDynamicLights() {
  if (pointLights_.empty() || !player_ || !boss_)
    return;

  PointLight *pLight = pointLights_[0].get();
  pLight->isActive = 1;

  Vector3 pPos = player_->GetTranslate();
  Vector3 bPos = boss_->GetGlobalTransform().translate;

  Vector3 midPos = Math::Add(pPos, bPos);
  midPos.x *= 0.5f;
  midPos.z *= 0.5f;

  // Enemyのジャンプによる光源の急激な上下動を防ぐため、
  // 光源のY軸（高さ）は上空からの見下ろしに最適な固定値とする。
  // ボスの通常時の頭上より少し高い位置（35.0f〜40.0fなど）
  midPos.y = 40.0f;

  pLight->position = midPos;

  // 2. 二人の距離に基づく範囲と強度の計算
  float distance = Math::Length(Math::Subtract(bPos, pPos));

  // 距離の 2.0倍程度を半径にする（最低でも40.0f程度は確保し、上空からの光が届くようにする）
  pLight->radius = (std::max)(40.0f, distance * 2.0f);

  // 距離が離れるほど強度を少し上げる
  pLight->intensity = 3.0f + (distance * 0.1f);
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
      if (Collision::IsOBBSphereCollision(stomp->GetFinalExplosionOBB(),
                                          playerColliderSphere)) {
        if (player_->ApplyDamage(stomp->GetFinalExplosionDamage())) {
          stomp->SetDealtFinalDamage(true);
        }
      }
    }
  }

  // 敵部位との接触判定
  auto checkHit = [&](auto *part) {
    if (!part || part->IsCompletelyDead())
      return;
    if (Collision::IsOBBSphereCollision(part->GetOBB(), playerColliderSphere)) {
      player_->ApplyDamage(kDamagePartToPlayer);
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
      if (part->ApplyDamage(kDamageMeleeToEnemy)) {
        if (part->GetHP() <= 0) {
          Vector3 attackDir = Math::Normalize(
              Math::Subtract(part->GetTransform().translate, playerPos));
          part->OnDestroyed(attackDir,
                            EnemyParameters::GetInstance()->GetBlowSpeed());
        }
        Vector3 scatterDir = Math::Normalize(
            Math::Subtract(part->GetTransform().translate, playerPos));
        part->ScatterAt(
            Math::Multiply(kMeleeScatterSpeedMultiplier, scatterDir),
            part->GetOBB());
      }
    }

    // マシンガン
    MachineGunBullet *bullets = player_->GetMachineGunBullets();
    for (int i = 0; i < Player::GetMaxMachineGunBullets(); ++i) {
      if (!bullets[i].isActive)
        continue;
      Sphere bulletSphere = {bullets[i].position, kMachineGunBulletRadius};
      if (Collision::IsOBBSphereCollision(part->GetOBB(), bulletSphere)) {
        bullets[i].isActive = false;
        if (part->ApplyDamage(kDamageMachineGunToEnemy) && part->GetHP() <= 0) {
          part->OnDestroyed(Math::Normalize(bullets[i].velocity),
                            EnemyParameters::GetInstance()->GetBlowSpeed());
        }
      }
    }

    // ミサイル
    MissileData *missiles = player_->GetMissiles();
    for (int i = 0; i < Player::GetMaxMissiles(); ++i) {
      if (!missiles[i].isActive)
        continue;
      Sphere missileSphere = {missiles[i].position, kMissileRadius};
      if (Collision::IsOBBSphereCollision(part->GetOBB(), missileSphere)) {
        missiles[i].isActive = false;
        if (part->ApplyDamage(kDamageMissileToEnemy) && part->GetHP() <= 0) {
          part->OnDestroyed(Math::Normalize(missiles[i].velocity),
                            EnemyParameters::GetInstance()->GetBlowSpeed());
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
          projectile->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, b1),
                                projectile->GetOBB());
          target->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, b2),
                            target->GetOBB());
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
      impactOBB.size = {attackSphere.radius, attackSphere.radius, attackSphere.radius};
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
        if (Collision::IsOBBCollision(stomp->GetFinalExplosionOBB(), bOBB)) {
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
        building->ScatterAt(i, Math::Multiply(kCollisionScatterMultiplier, vel), partOBB);

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
        part->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, vel),
                        partOBB);
      }
      // 飛んだ建物は部位に当たったので削れる
      Vector3 velForScatter = building->GetBlowVelocity(buildingIdx);
      building->ScatterAt(buildingIdx, Math::Multiply(kCollisionScatterMultiplier, velForScatter), partOBB);
      
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
        building->ScatterAt(ti, Math::Multiply(kCollisionScatterMultiplier, flyingVel), flyingOBB);
        building->ScatterAt(fi, Math::Multiply(kCollisionScatterMultiplier, Math::Multiply(-1.0f, flyingVel)), targetOBB);

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
