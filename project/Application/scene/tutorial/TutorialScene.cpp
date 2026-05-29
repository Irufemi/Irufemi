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

    int GetTutorialUIIndex(TutorialPhase phase) {
        switch (phase) {
            case TutorialPhase::MoveWASD:       return 0;
            case TutorialPhase::Dodge:          return 1;
            case TutorialPhase::MeleeAttack:    return 2;
            case TutorialPhase::GunAttack:      return 3;
            case TutorialPhase::KarakuriCharge: return 4;
            case TutorialPhase::EnhancedDodge:  return 5;
            case TutorialPhase::MissileAttack:  return 6;
            case TutorialPhase::MissileFiredPause: return 6;
            case TutorialPhase::MissileHitFocus: return 6;
            case TutorialPhase::BuildingSpawnFocus: return 7;
            case TutorialPhase::BuildingReadyFocus: return 7;
            case TutorialPhase::BuildingAttack: return 7;
            case TutorialPhase::BuildingHitFocus: return 7;
            case TutorialPhase::PartsExplanation: return 8;
            case TutorialPhase::ViewSwitch: return 9;
            default: return -1;
        }
    }
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

    for (int i = 0; i < 10; ++i) {
        tutorialUISprites_[i] = std::make_unique<Sprite>();
        std::string path = "resources/texture/inGame/tutorial_" + std::to_string(i + 1) + ".png";
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
        sprite->Initialize("resources/texture/inGame/key_" + keyName + ".png");
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

    // SPACEキーの初期化 (元に戻す)
    auto initSpaceKey = [](std::unique_ptr<Sprite>& sprite, float x, float y) {
        sprite = std::make_unique<Sprite>();
        sprite->Initialize("resources/texture/inGame/key_SPACE.png");
        sprite->SetAnchor(0.5f, 0.5f);
        sprite->SetPosition(x, y);
        sprite->SetSize(260.0f * 0.7f, 80.0f * 0.7f); // 少し小さめに
        // デフォルトはグレー
        sprite->SetColor({0.3f, 0.3f, 0.3f, 1.0f});
    };
    initSpaceKey(keySpaceSprite_, centerX, baseY);

    // ESCキーの初期化
    keyEscSprite_ = std::make_unique<Sprite>();
    keyEscSprite_->Initialize("resources/texture/inGame/key_ESC.png");
    keyEscSprite_->SetAnchor(0.0f, 0.0f);
    keyEscSprite_->SetPosition(20.0f, 20.0f);
    keyEscSprite_->SetSize(64.0f, 64.0f);
    keyEscSprite_->SetColor({0.8f, 0.8f, 0.8f, 1.0f});

    currentPhase_ = TutorialPhase::MoveWASD;
}

void TutorialScene::Update() {
    InputManager* input = engine_->GetInputManager();

    if (input && (input->IsKeyPressed(VK_ESCAPE) || input->StartPressed())) {
        engine_->GetSceneManager()->PushScene("Pause");
        return;
    }

    UpdateTutorialState();

    // フェーズに応じてプレイヤーの操作を制限する
    if (player_) {
        bool allowDodge       = currentPhase_ >= TutorialPhase::Dodge;
        bool allowMelee       = currentPhase_ >= TutorialPhase::MeleeAttack;
        // 右クリック（銃/ミサイル）: GunAttackで解禁 → KarakuriCharge/EnhancedDodgeで再封印 → MissileAttackで再解禁
        bool allowGunOrMissile = (currentPhase_ == TutorialPhase::GunAttack) ||
                                 (currentPhase_ >= TutorialPhase::MissileAttack);
        bool allowKarakuri    = currentPhase_ >= TutorialPhase::KarakuriCharge;
        bool allowViewSwitch  = currentPhase_ >= TutorialPhase::ViewSwitch;

        player_->SetAllowDodge(allowDodge);
        player_->SetAllowMelee(allowMelee);
        player_->SetAllowGunOrMissile(allowGunOrMissile);
        player_->SetAllowKarakuriCharge(allowKarakuri);
        player_->SetAllowViewSwitch(allowViewSwitch);
    }

    // MissileFiredPause: 0.0s~0.5s=通常（発射を見せる）、0.5s~0.8s=凍結（カメラ切替）、1.0s~=再開
    bool freezeGameplay = (currentPhase_ == TutorialPhase::MissileFiredPause && cinematicTimer_ >= 0.5f && cinematicTimer_ < 0.7f);

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
        if (!freezeGameplay) {
            player_->Update();
        }
    }

    if (boss_ && !freezeGameplay) {
        boss_->Update(player_.get());
    }

    if (!freezeGameplay) {
        CheckAllCollisions();
    }

    if (field_) {
        field_->Update();
    }

    skydome_->Update();
    if (player_ && boss_) {
        dynamicArenaLight_->Update(player_->GetTranslate(), boss_->GetTargetPosition());
    }

    // カメラの演出（BaseScene::Updateの前に更新する）
    Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
    if (activeCamera) {
        if ((currentPhase_ == TutorialPhase::MissileFiredPause && cinematicTimer_ >= 0.5f) || currentPhase_ == TutorialPhase::MissileHitFocus) {
            if (boss_) {
                // ボスからかなり離れて見下ろすめっちゃ引きのカメラ
                Vector3 bossPos = boss_->GetTargetPosition();
                Vector3 camPos = bossPos;
                camPos.y += 30.0f;
                camPos.z -= 80.0f; // 手前に引く
                
                activeCamera->SetTranslate(camPos);
                
                Vector3 diff = {
                    bossPos.x - camPos.x,
                    bossPos.y - camPos.y,
                    bossPos.z - camPos.z
                };
                float xzLen = std::sqrt(diff.x * diff.x + diff.z * diff.z);
                float pitch = -std::atan2(diff.y, xzLen);
                float yaw = std::atan2(diff.x, diff.z);
                
                activeCamera->SetRotate({ pitch, yaw, 0.0f });
                activeCamera->UpdateMatrix();
            }
        }
        else if (currentPhase_ == TutorialPhase::BuildingSpawnFocus) {
            if (field_ && field_->GetBuilding()) {
                Vector3 actualBuildingPos = field_->GetBuilding()->GetBuildingPosition(0);
                
                // ビルの根本近くにカメラを置く
                Vector3 camPos = spawnedBuildingPos_;
                camPos.y += 5.0f;  // 地上少し上
                camPos.z -= 25.0f; // ビルの手前
                camPos.x += 15.0f; // 少し横から
                
                activeCamera->SetTranslate(camPos);
                
                // ビルの中心付近を見上げる
                Vector3 targetPos = actualBuildingPos;
                targetPos.y += 15.0f;
                
                Vector3 diff = {
                    targetPos.x - camPos.x,
                    targetPos.y - camPos.y,
                    targetPos.z - camPos.z
                };
                
                float xzLen = std::sqrt(diff.x * diff.x + diff.z * diff.z);
                float pitch = -std::atan2(diff.y, xzLen);
                float yaw = std::atan2(diff.x, diff.z);
                
                activeCamera->SetRotate({ pitch, yaw, 0.0f });
                activeCamera->UpdateMatrix();
            }
        }
        else if (currentPhase_ == TutorialPhase::BuildingReadyFocus) {
            if (boss_) {
                // ビルとボスが映る少し引きのカメラ
                Vector3 bossPos = boss_->GetTargetPosition();
                Vector3 camPos = spawnedBuildingPos_; // ビルの位置を基準に引く
                camPos.y += 20.0f;
                camPos.z -= 60.0f;
                camPos.x -= 40.0f; // もっと左側に（-10.0fから-40.0fへ変更）
                
                activeCamera->SetTranslate(camPos);
                
                // ボスとビルの真ん中あたりを見る
                Vector3 targetPos = bossPos;
                targetPos.y += 10.0f;
                
                Vector3 diff = {
                    targetPos.x - camPos.x,
                    targetPos.y - camPos.y,
                    targetPos.z - camPos.z
                };
                float xzLen = std::sqrt(diff.x * diff.x + diff.z * diff.z);
                float pitch = -std::atan2(diff.y, xzLen);
                float yaw = std::atan2(diff.x, diff.z);
                
                activeCamera->SetRotate({ pitch, yaw, 0.0f });
                activeCamera->UpdateMatrix();
            }
        }
        else if (currentPhase_ == TutorialPhase::BuildingHitFocus) {
            if (boss_) {
                // ビルがボスに当たる様子を映す引きカメラ
                Vector3 bossPos = boss_->GetTargetPosition();
                Vector3 camPos = bossPos;
                camPos.y += 25.0f;
                camPos.z -= 70.0f;
                camPos.x += 10.0f;
                
                activeCamera->SetTranslate(camPos);
                
                Vector3 diff = {
                    bossPos.x - camPos.x,
                    bossPos.y - camPos.y,
                    bossPos.z - camPos.z
                };
                float xzLen = std::sqrt(diff.x * diff.x + diff.z * diff.z);
                float pitch = -std::atan2(diff.y, xzLen);
                float yaw = std::atan2(diff.x, diff.z);
                
                activeCamera->SetRotate({ pitch, yaw, 0.0f });
                activeCamera->UpdateMatrix();
            }
        }
    }

    BaseScene::Update();
    engine_->GetDrawManager()->SetEnvironmentMap(engine_->GetTextureManager()->GetWhiteCubeMapHandle());

    int uiIndex = GetTutorialUIIndex(currentPhase_);

    if (uiIndex >= 0 && uiIndex < 10) {
        if (tutorialUISprites_[uiIndex]) {
            tutorialUISprites_[uiIndex]->Update();
        }
    }

    // WASDキーの更新
    if (currentPhase_ == TutorialPhase::MoveWASD) {
        if (keyWSprite_) keyWSprite_->Update();
        if (keyASprite_) keyASprite_->Update();
        if (keySSprite_) keySSprite_->Update();
        if (keyDSprite_) keyDSprite_->Update();
    }

    // SPACEキーの更新
    if (currentPhase_ == TutorialPhase::BuildingReadyFocus || currentPhase_ == TutorialPhase::PartsExplanation || currentPhase_ == TutorialPhase::ViewSwitch) {
        if (keySpaceSprite_) keySpaceSprite_->Update();
    }

    if (keyEscSprite_) {
        keyEscSprite_->Update();
    }
}

void TutorialScene::Draw() {
    if (skydome_) skydome_->Draw();
    if (field_) field_->Draw();
    if (player_) player_->Draw();
    if (boss_) boss_->Draw(engine_);
    if (player_) player_->DrawParticles();

    bool hidePlayerUI = (player_ && player_->IsCinematicMode()) ||
                        currentPhase_ == TutorialPhase::MissileFiredPause ||
                        currentPhase_ == TutorialPhase::MissileHitFocus;
    if (player_ && !hidePlayerUI) {
        player_->Draw2DUI(boss_.get());
        bool isPaused = (engine_->GetSceneManager()->GetCurrent() == "Pause");
        player_->Draw3DUI(boss_.get(), true, isPaused);
    }

    int uiIndex = GetTutorialUIIndex(currentPhase_);
    if (uiIndex >= 0 && uiIndex < 10) {
        if (tutorialUISprites_[uiIndex]) {
            tutorialUISprites_[uiIndex]->Draw();
        }
    }

    // WASDキーの描画
    if (currentPhase_ == TutorialPhase::MoveWASD) {
        if (keyWSprite_) keyWSprite_->Draw();
        if (keyASprite_) keyASprite_->Draw();
        if (keySSprite_) keySSprite_->Draw();
        if (keyDSprite_) keyDSprite_->Draw();
    }

    // SPACEキーの描画
    if (currentPhase_ == TutorialPhase::BuildingReadyFocus || currentPhase_ == TutorialPhase::PartsExplanation || currentPhase_ == TutorialPhase::ViewSwitch) {
        if (keySpaceSprite_) keySpaceSprite_->Draw();
    }

    if (keyEscSprite_) keyEscSprite_->Draw();
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
        if (keyWSprite_) keyWSprite_->SetColor(input->IsKeyDownDIK(DIK_W) || hasPressedW_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});
        if (keyASprite_) keyASprite_->SetColor(input->IsKeyDownDIK(DIK_A) || hasPressedA_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});
        if (keySSprite_) keySSprite_->SetColor(input->IsKeyDownDIK(DIK_S) || hasPressedS_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});
        if (keyDSprite_) keyDSprite_->SetColor(input->IsKeyDownDIK(DIK_D) || hasPressedD_ ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});

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
        // ミサイルが発射されたか検出する
        if (player_) {
            MissileData* missiles = player_->GetMissiles();
            bool hasMissileActive = false;
            for (int i = 0; i < Player::GetMaxMissiles(); ++i) {
                if (missiles[i].isActive) {
                    hasMissileActive = true;
                    break;
                }
            }
            if (hasMissileActive) {
                currentPhase_ = TutorialPhase::MissileFiredPause;
                cinematicTimer_ = 0.0f;
                // 最初の0.5秒は発射シーンを見せるのでcinematicModeはまだ設定しない
            }
        }
        break;

    case TutorialPhase::MissileFiredPause:
        cinematicTimer_ += engine_->GetDeltaTime();
        // 0.5秒経過でシネマティックモード（カメラ切替＋操作停止）
        if (cinematicTimer_ >= 0.5f && player_ && !player_->IsCinematicMode()) {
            player_->SetCinematicMode(true);
        }
        // 着弾したら次のフェーズへ
        if (hasHitMissile_) {
            currentPhase_ = TutorialPhase::MissileHitFocus;
            cinematicTimer_ = 0.0f;
        }
        break;

    case TutorialPhase::MissileHitFocus:
        cinematicTimer_ += engine_->GetDeltaTime();
        if (cinematicTimer_ > 3.0f) {
            currentPhase_ = TutorialPhase::BuildingSpawnFocus;
            if (field_ && field_->GetBuilding()) {
                // 建物を一つだけボスの少し遠くに配置する
                Vector3 bossPos = boss_->GetTargetPosition();
                bossPos.z -= 60.0f; // ボスから大きく離す（手前に）
                bossPos.x += 20.0f; // 横にずらす
                field_->GetBuilding()->ClearAndAddSingleBuilding(bossPos);
                spawnedBuildingPos_ = bossPos;
            }
        }
        break;

    case TutorialPhase::BuildingSpawnFocus:
        if (field_ && field_->GetBuilding()) {
            if (!field_->GetBuilding()->IsBuildingSpawning(0)) {
                currentPhase_ = TutorialPhase::BuildingReadyFocus;
                cinematicTimer_ = 0.0f;
            } else {
                if (player_) player_->SetCinematicMode(true);
            }
        } else {
            currentPhase_ = TutorialPhase::BuildingReadyFocus;
        }
        break;

    case TutorialPhase::BuildingReadyFocus:
        if (keySpaceSprite_) keySpaceSprite_->SetColor(input->IsKeyDownDIK(DIK_SPACE) ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});
        if (input->IsKeyReleasedDIK(DIK_SPACE)) {
            currentPhase_ = TutorialPhase::BuildingAttack;
            if (player_) player_->SetCinematicMode(false);
        }
        break;

    case TutorialPhase::BuildingAttack:
        if (hasBuildingHitEnemy_) {
            currentPhase_ = TutorialPhase::BuildingHitFocus;
            cinematicTimer_ = 0.0f;
            if (player_) player_->SetCinematicMode(true);
        } else if (field_ && field_->GetBuilding()) {
            // 建物が消滅してしまって敵に当たっていない場合は再生成する
            if (field_->GetBuilding()->GetBuildingCount() > 0 && 
                field_->GetBuilding()->IsBuildingDestroyed(0)) {
                Vector3 bossPos = boss_->GetTargetPosition();
                bossPos.z -= 60.0f;
                bossPos.x += 20.0f;
                field_->GetBuilding()->ClearAndAddSingleBuilding(bossPos);
            }
        }
        break;

    case TutorialPhase::BuildingHitFocus:
        cinematicTimer_ += engine_->GetDeltaTime();
        if (cinematicTimer_ > 3.0f) {
            currentPhase_ = TutorialPhase::PartsExplanation;
            if (player_) player_->SetCinematicMode(false);
        }
        break;

    case TutorialPhase::PartsExplanation:
        if (keySpaceSprite_) keySpaceSprite_->SetColor(input->IsKeyDownDIK(DIK_SPACE) ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});
        if (input->IsKeyReleasedDIK(DIK_SPACE)) {
            currentPhase_ = TutorialPhase::ViewSwitch;
        }
        break;

    case TutorialPhase::ViewSwitch:
        if (keySpaceSprite_) keySpaceSprite_->SetColor(input->IsKeyDownDIK(DIK_SPACE) ? Vector4{0.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.3f, 0.3f, 0.3f, 1.0f});
        if (input->IsKeyReleasedDIK(DIK_SPACE)) {
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
    CheckPlayerBuildingCollisions();
    CheckFlyingBuildingsVsEnemyCollisions();
}

void TutorialScene::CheckEnemyToPlayerCollisions() {
    // チュートリアルでは敵は攻撃しないため、部位接触による押し出しのみ（ダメージなし）
    Sphere playerColliderSphere;
    playerColliderSphere.center = player_->GetCollider().center;
    playerColliderSphere.radius = player_->GetCollider().radius;

    auto checkHit = [&](auto *part) {
        if (!part || part->IsCompletelyDead()) return;
        OBB partOBB = part->GetOBB();
        if (Collision::IsOBBSphereCollision(partOBB, playerColliderSphere)) {
            bool isTackle = (boss_->GetState() == EnemyState::Attack_Tackle);

            // 押し出し処理
            Vector3 diff = Math::Subtract(player_->GetTranslate(), partOBB.center);
            float bestPushEval = 1e10f;
            float bestActualPushDist = 0.0f;
            Vector3 bestPushDir = {0.0f, 0.0f, 0.0f};

            for (int axis = 0; axis < 3; ++axis) {
                if (axis == 1) continue; // Y軸は無視

                float proj = Math::Dot(diff, partOBB.orientations[axis]);
                float halfSize = (axis == 0) ? partOBB.size.x : partOBB.size.z;
                
                float actualPenetration = 0.0f;
                float evalPenetration = 0.0f;
                Vector3 pushDir = {0.0f, 0.0f, 0.0f};

                if (isTackle && axis == 0) {
                    // --- 突進中専用：X軸（左右）へ強制的に逃がす ---
                    Vector3 bossPos = boss_->GetTargetPosition();
                    Vector3 diffFromBoss = Math::Subtract(player_->GetTranslate(), bossPos);
                    float signX = (Math::Dot(diffFromBoss, partOBB.orientations[0]) >= 0.0f) ? 1.0f : -1.0f;
                    
                    if (signX > 0.0f) {
                        actualPenetration = (halfSize + playerColliderSphere.radius) - proj;
                        pushDir = partOBB.orientations[0];
                    } else {
                        actualPenetration = (halfSize + playerColliderSphere.radius) + proj;
                        pushDir = Math::Multiply(-1.0f, partOBB.orientations[0]);
                    }
                    
                    actualPenetration += 3.0f; // 外側へ弾き出すボーナス
                    evalPenetration = actualPenetration;
                } else {
                    // --- 通常時の押し出し（突進中のZ軸も含む） ---
                    actualPenetration = (halfSize + playerColliderSphere.radius) - std::abs(proj);
                    float sign = (proj >= 0.0f) ? 1.0f : -1.0f;
                    pushDir = Math::Multiply(sign, partOBB.orientations[axis]);
                    
                    if (isTackle && axis == 2) {
                        // 突進中はZ軸へ絶対に押し出されないようペナルティ
                        evalPenetration = actualPenetration * 1000.0f;
                    } else {
                        // 通常はそのままの距離で評価
                        evalPenetration = actualPenetration;
                    }
                }

                if (actualPenetration > 0.0f && evalPenetration < bestPushEval) {
                    bestPushEval = evalPenetration;
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
                player_->OnMeleeHit();
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

