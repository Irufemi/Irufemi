#include "TutorialScene.h"
#include <algorithm>
#include <format>
#include <dinput.h>

#include "Framework/SceneManager.h"
#include "Irufemi.h"

#include "contents/light/DynamicArenaLight.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Graphics/Camera/DebugCamera.h"
#include "Graphics/PostProcess/PostProcessManager.h"

#include "actors/enemy/Enemy.h"
#include "actors/enemy/Body/Body.h"
#include "actors/player/Player.h"
#include "contents/field/Field.h"
#include "contents/field/building/building.h"
#include "contents/skydome/Skydome.h"

#include "contents/ui/EnemyHPBar.h"
#include "contents/ui/EnemyPartHPBar.h"
#include "contents/ui/PlayerHPBar.h"

#include "Renderer/Object2D/Sprite/Sprite.h"

#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/IrufemiEngine.h"

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif

namespace {
    const Vector3 kDefaultCameraPos = {0.0f, 0.0f, -10.0f};
}

TutorialScene::TutorialScene() {}
TutorialScene::~TutorialScene() {}

void TutorialScene::Initialize(IrufemiEngine* engine) {
    BaseScene::Initialize(engine);

    Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
    activeCamera->SetTranslate(kDefaultCameraPos);
    activeCamera->UpdateMatrix();

    // プレイヤーの初期化
    player_ = std::make_unique<Player>();
    player_->Initialize(engine_->GetInputManager(), engine_);

    // 敵の初期化
    boss_ = std::make_unique<Enemy>();
    boss_->Initialize(engine_);
    boss_->SetIsSandbagMode(true); // サンドバッグ化

    // フィールドの初期化
    field_ = std::make_unique<Field>(engine_);
    field_->Initialize();

    // 初期状態では建物を配置しない
    field_->GetBuilding()->ClearAllBuildings();

    skydome_ = std::make_unique<Skydome>();
    skydome_->Initialize();

    dynamicArenaLight_ = std::make_unique<DynamicArenaLight>();
    dynamicArenaLight_->Initialize(engine_, areaLights_);

    for (int i = 0; i < 9; ++i) {
        tutorialUISprites_[i] = std::make_unique<Sprite>();
        std::string path = "resources/UI/tutorial_" + std::to_string(i + 1) + ".png";
        tutorialUISprites_[i]->Initialize(path);
        tutorialUISprites_[i]->SetAnchor(0.5f, 0.0f); // 上部中央
        tutorialUISprites_[i]->SetPosition(1280.0f / 2.0f, 15.0f); // もう少し上に配置
        
        // 元の画像サイズから0.7倍に縮小
        auto size = tutorialUISprites_[i]->GetSize();
        tutorialUISprites_[i]->SetSize(size.x * 0.7f, size.y * 0.7f);
    }

    // WASDキーの初期化
    auto initKey = [](std::unique_ptr<Sprite>& sprite, const std::string& keyName, float x, float y) {
        sprite = std::make_unique<Sprite>();
        sprite->Initialize("resources/UI/key_" + keyName + ".png");
        sprite->SetAnchor(0.5f, 0.5f);
        sprite->SetPosition(x, y);
        sprite->SetSize(80.0f * 0.8f, 80.0f * 0.8f); // 少し小さめに
        // デフォルトはグレー
        sprite->SetColor({0.3f, 0.3f, 0.3f, 1.0f});
    };
    
    float centerX = 1280.0f / 2.0f;
    float baseY = 190.0f; // 説明UIの下
    initKey(keyWSprite_, "W", centerX, baseY);
    initKey(keyASprite_, "A", centerX - 70.0f, baseY + 70.0f);
    initKey(keySSprite_, "S", centerX, baseY + 70.0f);
    initKey(keyDSprite_, "D", centerX + 70.0f, baseY + 70.0f);

    currentPhase_ = TutorialPhase::MoveWASD;
}

void TutorialScene::Update() {
    InputManager* input = engine_->GetInputManager();

    if (input && (input->IsKeyPressed(VK_ESCAPE) || input->StartPressed())) {
        engine_->GetSceneManager()->PushScene("Pause");
        return;
    }

    UpdateTutorialState();

    if (player_) {
        if (boss_) {
            Vector3 bossPos = boss_->GetTargetPosition();
            player_->SetTargetPosition(bossPos);

            Vector3 playerPos = player_->GetTranslate();
            Vector3 toBoss = Math::Subtract(bossPos, playerPos);
            float len = Math::Length(toBoss);
            bool inScreen = false;

            if (len > 0.001f) {
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

    CheckAllCollisions();

    if (field_) {
        field_->Update();
    }

    skydome_->Update();
    if (player_ && boss_) {
        dynamicArenaLight_->Update(player_->GetTranslate(), boss_->GetTargetPosition());
    }

    BaseScene::Update();
    engine_->GetDrawManager()->SetEnvironmentMap(engine_->GetTextureManager()->GetWhiteCubeMapHandle());

    int phaseIndex = static_cast<int>(currentPhase_);
    if (phaseIndex >= 0 && phaseIndex < 9) {
        if (tutorialUISprites_[phaseIndex]) {
            tutorialUISprites_[phaseIndex]->Update();
        }
    }

    // WASDキーの更新
    if (currentPhase_ == TutorialPhase::MoveWASD) {
        if (keyWSprite_) keyWSprite_->Update();
        if (keyASprite_) keyASprite_->Update();
        if (keySSprite_) keySSprite_->Update();
        if (keyDSprite_) keyDSprite_->Update();
    }
}

void TutorialScene::Draw() {
    if (skydome_) skydome_->Draw();
    if (field_) field_->Draw();
    if (player_) player_->Draw();
    if (boss_) boss_->Draw(engine_);
    if (player_) player_->DrawParticles();

    if (player_) {
        player_->Draw2DUI(boss_.get());
        bool isPaused = (engine_->GetSceneManager()->GetCurrent() == "Pause");
        player_->Draw3DUI(boss_.get(), true, isPaused);
    }

    int phaseIndex = static_cast<int>(currentPhase_);
    if (phaseIndex >= 0 && phaseIndex < 9) {
        if (tutorialUISprites_[phaseIndex]) {
            tutorialUISprites_[phaseIndex]->Draw();
        }
    }

    // WASDキーの描画
    if (currentPhase_ == TutorialPhase::MoveWASD) {
        if (keyWSprite_) keyWSprite_->Draw();
        if (keyASprite_) keyASprite_->Draw();
        if (keySSprite_) keySSprite_->Draw();
        if (keyDSprite_) keyDSprite_->Draw();
    }
}

void TutorialScene::DrawDebugTab() {
#ifdef USE_IMGUI
    BaseScene::DrawDebugTab();
#endif
}

void TutorialScene::UpdateTutorialState() {
    InputManager* input = engine_->GetInputManager();

    // フェーズに応じたプレイヤーの制御
    if (currentPhase_ == TutorialPhase::KarakuriCharge || currentPhase_ > TutorialPhase::KarakuriCharge) {
        if (currentPhase_ > TutorialPhase::KarakuriCharge) {
            player_->ForceKarakuriCharge();
        }
    }

    // クールタイムの短縮
    if (currentPhase_ == TutorialPhase::Dodge || currentPhase_ == TutorialPhase::EnhancedDodge) {
        player_->ResetDodgeCooldown();
    }
    if (currentPhase_ == TutorialPhase::MeleeAttack || currentPhase_ == TutorialPhase::GunAttack || currentPhase_ == TutorialPhase::MissileAttack) {
        player_->ResetSkillCooldown();
    }

    switch (currentPhase_) {
    case TutorialPhase::MoveWASD:
        if (input->IsKeyPressedDIK(DIK_W)) hasPressedW_ = true;
        if (input->IsKeyPressedDIK(DIK_A)) hasPressedA_ = true;
        if (input->IsKeyPressedDIK(DIK_S)) hasPressedS_ = true;
        if (input->IsKeyPressedDIK(DIK_D)) hasPressedD_ = true;

        // 押されたら水色（点灯）、押されていなければグレー
        if (keyWSprite_) keyWSprite_->SetColor(hasPressedW_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.4f, 0.4f, 0.4f, 1.0f});
        if (keyASprite_) keyASprite_->SetColor(hasPressedA_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.4f, 0.4f, 0.4f, 1.0f});
        if (keySSprite_) keySSprite_->SetColor(hasPressedS_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.4f, 0.4f, 0.4f, 1.0f});
        if (keyDSprite_) keyDSprite_->SetColor(hasPressedD_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.4f, 0.4f, 0.4f, 1.0f});

        if (hasPressedW_ && hasPressedA_ && hasPressedS_ && hasPressedD_) {
            currentPhase_ = TutorialPhase::Dodge;
        }
        break;

    case TutorialPhase::Dodge:
        if (input->IsKeyPressedDIK(DIK_SPACE)) { // Space key
            currentPhase_ = TutorialPhase::MeleeAttack;
        }
        break;

    case TutorialPhase::MeleeAttack:
        if (hasHitMelee_) {
            currentPhase_ = TutorialPhase::GunAttack;
        }
        break;

    case TutorialPhase::GunAttack:
        if (hasHitGun_) {
            currentPhase_ = TutorialPhase::KarakuriCharge;
        }
        break;

    case TutorialPhase::KarakuriCharge:
        if (player_->IsKarakuriCharged()) {
            currentPhase_ = TutorialPhase::EnhancedDodge;
        }
        break;

    case TutorialPhase::EnhancedDodge:
        if (input->IsKeyPressedDIK(DIK_SPACE)) {
            currentPhase_ = TutorialPhase::MissileAttack;
        }
        break;

    case TutorialPhase::MissileAttack:
        if (hasHitMissile_) {
            currentPhase_ = TutorialPhase::BuildingAttack;
            if (field_ && field_->GetBuilding()) {
                // 建物を一つだけボスの少し遠くに配置する
                Vector3 bossPos = boss_->GetTargetPosition();
                bossPos.z -= 30.0f; // 少し手前に配置
                bossPos.x += 15.0f; // 横にずらす
                field_->GetBuilding()->ClearAndAddSingleBuilding(bossPos);
            }
        }
        break;

    case TutorialPhase::BuildingAttack:
        if (hasBuildingHitEnemy_) {
            currentPhase_ = TutorialPhase::PartsExplanation;
        } else if (field_ && field_->GetBuilding()) {
            // 建物が消滅してしまって敵に当たっていない場合は再生成する
            if (field_->GetBuilding()->GetBuildingCount() > 0 && 
                field_->GetBuilding()->IsBuildingDestroyed(0)) {
                Vector3 bossPos = boss_->GetTargetPosition();
                bossPos.z -= 30.0f;
                bossPos.x += 15.0f;
                field_->GetBuilding()->ClearAndAddSingleBuilding(bossPos);
            }
        }
        break;

    case TutorialPhase::PartsExplanation:
        if (input->IsKeyPressedDIK(DIK_SPACE)) {
            currentPhase_ = TutorialPhase::Done;
        }
        break;

    case TutorialPhase::Done:
        if (engine_ && engine_->GetSceneManager()) {
            engine_->GetSceneManager()->TransitionTo("InGame", SceneTransition::Type::RadialBlur, 1.5f);
        }
        break;
    }
}

void TutorialScene::CheckAllCollisions() {
    if (!player_ || !boss_ || !isCollisionEnabled_) return;
    CheckEnemyToPlayerCollisions();
    CheckPlayerToEnemyCollisions();
    CheckFlyingPartsCollisions();
    CheckPlayerBuildingCollisions();
    CheckEnemyBuildingCollisions();
    CheckFlyingPartsBuildingCollisions();
    CheckFlyingBuildingsVsEnemyCollisions();
    CheckFlyingBuildingsVsBuildingsCollisions();
}

void TutorialScene::CheckEnemyToPlayerCollisions() {
    // チュートリアルでは敵は攻撃しないため、部位接触による押し出し・ダメージのみ
    Sphere playerColliderSphere;
    playerColliderSphere.center = player_->GetCollider().center;
    playerColliderSphere.radius = player_->GetCollider().radius;

    auto checkHit = [&](auto *part) {
        if (!part || part->IsCompletelyDead()) return;
        if (Collision::IsOBBSphereCollision(part->GetOBB(), playerColliderSphere)) {
            if (player_->ApplyDamage(10)) { // ダメージは10に固定
                if (part->IsBlownAway()) {
                    Vector3 toPlayer = Math::Subtract(playerColliderSphere.center, part->GetOBB().center);
                    toPlayer.y = 0.0f;
                    Vector3 normal = Math::Normalize(toPlayer);
                    if (Math::Length(normal) < 0.001f) normal = {0.0f, 0.0f, 1.0f};

                    Vector3 vel = part->GetBlowVelocity();
                    float dot = Math::Dot(vel, normal);
                    if (dot > 0.0f) {
                        Vector3 reflect = Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
                        part->SetBlowVelocity(reflect);
                    } else if (Math::Length(vel) < 0.001f) {
                        part->SetBlowVelocity(Math::Multiply(-2.0f, normal));
                    }
                }
            }
        }
    };
    for (int i = 0; i < 3; ++i) checkHit(boss_->GetBody(i));
    checkHit(boss_->GetHeadLeft());
    checkHit(boss_->GetHeadMid());
    checkHit(boss_->GetHeadRight());
}

void TutorialScene::CheckPlayerToEnemyCollisions() {
    const AttackCollision &attackCol = player_->GetAttackCollision();
    Sphere attackSphere;
    attackSphere.center = attackCol.center;
    attackSphere.radius = attackCol.radius;
    Vector3 playerPos = player_->GetTranslate();

    auto checkAndDamage = [&](auto *part) {
        if (!part || part->GetHP() <= 0 || part->IsBlownAway()) return;

        // 近接攻撃
        if (attackCol.isActive && Collision::IsOBBSphereCollision(part->GetOBB(), attackSphere)) {
            int damage = player_->GetDamageMelee();
            if (player_->IsKarakuriCharged()) damage = static_cast<int>(damage * player_->GetDamageMeleeChargeMultiplier());
            if (part->ApplyDamage(damage)) {
                hasHitMelee_ = true;
                if (part->GetHP() <= 0) {
                    part->SetHP(100); // チュートリアル中は部位が壊れないようにHPを回復
                }
                Vector3 scatterDir = Math::Normalize(Math::Subtract(part->GetTransform().translate, playerPos));
                OBB hitArea;
                hitArea.center = attackSphere.center;
                hitArea.orientations[0] = {1.0f, 0.0f, 0.0f}; hitArea.orientations[1] = {0.0f, 1.0f, 0.0f}; hitArea.orientations[2] = {0.0f, 0.0f, 1.0f};
                hitArea.size = {attackSphere.radius, attackSphere.radius, attackSphere.radius};
                part->ScatterAt(Math::Multiply(1.0f, scatterDir), hitArea);
            }
        }

        if (part->GetHP() <= 0) return;

        // マシンガン
        auto bullets = player_->GetMachineGunBullets();
        for (int i = 0; i < Player::GetMaxMachineGunBullets(); ++i) {
            if (!bullets[i].isActive) continue;
            Sphere bulletSphere = {bullets[i].position, 1.0f};
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
                hasHitGun_ = true;
                int damage = player_->GetDamageMachineGun();
                if (player_->IsKarakuriCharged()) damage = static_cast<int>(damage * player_->GetDamageMachineGunChargeMultiplier());
                if (part->ApplyDamage(damage)) {
                    if (part->GetHP() <= 0) {
                        part->SetHP(100);
                    }
                    break;
                }
            }
        }

        if (part->GetHP() <= 0) return;

        // ミサイル
        auto missiles = player_->GetMissiles();
        for (int i = 0; i < Player::GetMaxMissiles(); ++i) {
            if (!missiles[i].isActive) continue;
            Sphere missileSphere = {missiles[i].position, 2.0f};
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
                hasHitMissile_ = true;
                int damage = player_->GetDamageMissile();
                if (player_->IsKarakuriCharged()) damage = static_cast<int>(damage * player_->GetDamageMissileChargeMultiplier());
                if (part->ApplyDamage(damage)) {
                    if (part->GetHP() <= 0) {
                        part->SetHP(100);
                    }
                    break;
                }
            }
        }
    };

    for (int i = 0; i < 3; ++i) checkAndDamage(boss_->GetBody(i));
    checkAndDamage(boss_->GetHeadLeft());
    checkAndDamage(boss_->GetHeadMid());
    checkAndDamage(boss_->GetHeadRight());
}

void TutorialScene::CheckFlyingPartsCollisions() {
    // チュートリアルなので簡易的に部位同士の衝突を実装
    auto checkProjectile = [&](auto *projectile) {
        if (!projectile || !projectile->IsBlownAway() || projectile->IsCompletelyDead()) return;
        if (projectile->GetBlowTimer() < 0.2f) return;
        OBB projectileOBB = projectile->GetOBB();

        auto checkTarget = [&](auto *target) {
            if (!target || target->GetHP() <= 0 || target->IsBlownAway()) return;
            if (Collision::IsOBBCollision(projectileOBB, target->GetOBB())) {
                Vector3 vel = projectile->GetBlowVelocity();
                Vector3 diff = Math::Subtract(projectile->GetTransform().translate, target->GetTransform().translate);
                Vector3 normal = Math::Normalize(Vector3{diff.x, 0.0f, diff.z});
                if (Math::Length(normal) < 0.001f) normal = {0.0f, 0.0f, 1.0f};

                float dot = Math::Dot(vel, normal);
                if (dot < 0.0f) {
                    target->ApplyDamage(250);
                    Vector3 reflect = Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
                    reflect.y = 0.0f;
                    projectile->SetBlowVelocity(reflect);
                    if (target->GetHP() <= 0) target->OnDestroyed(Math::Normalize(vel), 0.5f);
                    target->ScatterAt(Math::Multiply(-0.5f, vel), target->GetOBB());
                    projectile->ScatterAt(Math::Multiply(-0.5f, reflect), projectile->GetOBB());
                }
            }
        };

        for (int i = 0; i < 3; ++i) checkTarget(boss_->GetBody(i));
        checkTarget(boss_->GetHeadLeft());
        checkTarget(boss_->GetHeadMid());
        checkTarget(boss_->GetHeadRight());
    };

    for (int i = 0; i < 3; ++i) checkProjectile(boss_->GetBody(i));
    checkProjectile(boss_->GetHeadLeft());
    checkProjectile(boss_->GetHeadMid());
    checkProjectile(boss_->GetHeadRight());
}

void TutorialScene::CheckPlayerBuildingCollisions() {
    Building *building = field_ ? field_->GetBuilding() : nullptr;
    if (!building || !player_) return;

    Vector3 playerPos = player_->GetTranslate();
    float playerRadius = player_->GetCollider().radius;
    building->ResolvePlayerCollision(playerPos, playerRadius);
    player_->SetTranslate(playerPos);

    const AttackCollision &attackCol = player_->GetAttackCollision();
    Sphere attackSphere;
    attackSphere.center = attackCol.center;
    attackSphere.radius = attackCol.radius;

    for (int i = 0; i < building->GetBuildingCount(); ++i) {
        if (!building->IsBuildingAlive(i)) continue;
        OBB bOBB = building->GetBuildingOBB(i);

        if (attackCol.isActive && Collision::IsOBBSphereCollision(bOBB, attackSphere)) {
            Vector3 attackDir = Math::Normalize(Math::Subtract(bOBB.center, playerPos));
            building->ApplyDamage(i, 16, attackDir, 0.5f);
            OBB impactOBB;
            impactOBB.center = attackSphere.center;
            impactOBB.orientations[0] = {1.0f, 0.0f, 0.0f}; impactOBB.orientations[1] = {0.0f, 1.0f, 0.0f}; impactOBB.orientations[2] = {0.0f, 0.0f, 1.0f};
            impactOBB.size = {attackSphere.radius, attackSphere.radius, attackSphere.radius};
            building->ScatterAt(i, Math::Multiply(3.0f, attackDir), impactOBB);
        }

        auto bullets = player_->GetMachineGunBullets();
        for (int b = 0; b < Player::GetMaxMachineGunBullets(); ++b) {
            if (!bullets[b].isActive) continue;
            Sphere bulletSphere = {bullets[b].position, 1.0f};
            if (Collision::IsOBBSphereCollision(bOBB, bulletSphere)) {
                bullets[b].isActive = false;
                Vector3 hitPoint;
                Segment segment = { Math::Subtract(bullets[b].position, bullets[b].velocity), bullets[b].velocity };
                if (Collision::GetOBBSegmentIntersection(bOBB, segment, hitPoint)) {
                    Vector3 pushDir = Math::Multiply(-1.0f, bullets[b].velocity);
                    float len = Math::Length(pushDir);
                    if (len > 0.001f) pushDir = Math::Normalize(pushDir);
                    else pushDir = { 0.0f, 1.0f, 0.0f };
                    hitPoint = Math::Add(hitPoint, Math::Multiply(0.4f, pushDir));
                } else {
                    hitPoint = Collision::GetOBBSphereClosestPoint(bOBB, bulletSphere, 0.4f);
                }
                player_->PlayExplosion(hitPoint, 0.25f);
                Vector3 attackDir = Math::Normalize(bullets[b].velocity);
                building->ApplyDamage(i, 6, attackDir, 0.3f);
            }
        }

        auto missiles = player_->GetMissiles();
        for (int m = 0; m < Player::GetMaxMissiles(); ++m) {
            if (!missiles[m].isActive) continue;
            Sphere missileSphere = {missiles[m].position, 2.0f};
            if (Collision::IsOBBSphereCollision(bOBB, missileSphere)) {
                missiles[m].isActive = false;
                Vector3 hitPoint;
                Segment segment = { Math::Subtract(missiles[m].position, missiles[m].velocity), missiles[m].velocity };
                if (Collision::GetOBBSegmentIntersection(bOBB, segment, hitPoint)) {
                    Vector3 pushDir = Math::Multiply(-1.0f, missiles[m].velocity);
                    float len = Math::Length(pushDir);
                    if (len > 0.001f) pushDir = Math::Normalize(pushDir);
                    else pushDir = { 0.0f, 1.0f, 0.0f };
                    hitPoint = Math::Add(hitPoint, Math::Multiply(1.0f, pushDir));
                } else {
                    hitPoint = Collision::GetOBBSphereClosestPoint(bOBB, missileSphere, 1.0f);
                }
                player_->PlayExplosion(hitPoint, 1.2f);
                Vector3 attackDir = Math::Normalize(missiles[m].velocity);
                building->ApplyDamage(i, 100, attackDir, 0.8f);
            }
        }
    }
}

void TutorialScene::CheckEnemyBuildingCollisions() {
    // チュートリアルなので敵から建物への攻撃判定は省略可能
}

void TutorialScene::CheckFlyingPartsBuildingCollisions() {
    Building *building = field_ ? field_->GetBuilding() : nullptr;
    if (!building || !boss_) return;

    auto checkProjectile = [&](auto *projectile) {
        if (!projectile || !projectile->IsBlownAway() || projectile->IsCompletelyDead()) return;
        if (projectile->GetBlowTimer() < 0.2f) return;
        OBB pOBB = projectile->GetOBB();

        for (int i = 0; i < building->GetBuildingCount(); ++i) {
            if (!building->IsBuildingAlive(i)) continue;
            OBB bOBB = building->GetBuildingOBB(i);

            if (Collision::IsOBBCollision(pOBB, bOBB)) {
                Vector3 vel = projectile->GetBlowVelocity();
                Vector3 diff = Math::Subtract(projectile->GetTransform().translate, bOBB.center);
                
                float bestPushDist = 1e10f;
                Vector3 bestPushDir = {0.0f, 0.0f, 0.0f};
                for (int axis = 0; axis < 3; ++axis) {
                    if (axis == 1) continue;
                    float proj = Math::Dot(diff, bOBB.orientations[axis]);
                    float halfSize = (axis == 0) ? bOBB.size.x : bOBB.size.z;
                    float penetration = halfSize - std::abs(proj);
                    if (penetration > 0.0f && penetration < bestPushDist) {
                        bestPushDist = penetration;
                        float sign = (proj >= 0.0f) ? 1.0f : -1.0f;
                        bestPushDir = Math::Multiply(sign, bOBB.orientations[axis]);
                    }
                }
                
                if (bestPushDist < 1e10f) {
                    float dot = Math::Dot(vel, bestPushDir);
                    if (dot < 0.0f) {
                        Vector3 reflect = Math::Subtract(vel, Math::Multiply(2.0f * dot, bestPushDir));
                        reflect.y = 0.0f;
                        projectile->SetBlowVelocity(reflect);
                        building->ApplyDamage(i, 50, Math::Normalize(vel), 0.5f);
                        building->ScatterAt(i, Math::Multiply(0.5f, vel), pOBB);
                        projectile->ScatterAt(Math::Multiply(-0.5f, vel), projectile->GetOBB());
                    }
                }
            }
        }
    };

    for (int i = 0; i < 3; ++i) checkProjectile(boss_->GetBody(i));
    checkProjectile(boss_->GetHeadLeft());
    checkProjectile(boss_->GetHeadMid());
    checkProjectile(boss_->GetHeadRight());
}

void TutorialScene::CheckFlyingBuildingsVsEnemyCollisions() {
    Building *building = field_ ? field_->GetBuilding() : nullptr;
    if (!building || !boss_) return;

    for (int i = 0; i < building->GetBuildingCount(); ++i) {
        if (!building->IsBuildingBlownAway(i)) continue;
        OBB bOBB = building->GetBuildingOBB(i);
        Vector3 bVel = building->GetBlowVelocity(i);

        auto checkAndDamage = [&](auto *part) {
            if (!part || part->GetHP() <= 0 || part->IsBlownAway()) return;
            if (Collision::IsOBBCollision(bOBB, part->GetOBB())) {
                building->MarkDestroyed(i);
                part->ApplyDamage(200);
                hasBuildingHitEnemy_ = true; // ★ チュートリアル進行フラグ
                if (part->GetHP() <= 0) {
                    part->SetHP(100); // チュートリアル中は部位が壊れないようにHPを回復
                }
                part->ScatterAt(Math::Multiply(0.5f, bVel), part->GetOBB());
            }
        };

        for (int p = 0; p < 3; ++p) checkAndDamage(boss_->GetBody(p));
        checkAndDamage(boss_->GetHeadLeft());
        checkAndDamage(boss_->GetHeadMid());
        checkAndDamage(boss_->GetHeadRight());
    }
}

void TutorialScene::CheckFlyingBuildingsVsBuildingsCollisions() {
    // チュートリアルでは建物は1つなので省略可能
}
