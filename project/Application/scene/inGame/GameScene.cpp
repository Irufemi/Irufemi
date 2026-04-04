#include "GameScene.h"
#include <format>

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
#include "Graphics/PostProcess/PostProcessManager.h"

#include "Engine/Core/Math/Geometry/Collision.h"

#include <Windows.h>

namespace {
    // --- ローカル定数（座標やベクトルなどの初期値） ---
    const Vector3 kDefaultCameraPos = { 0.0f, 0.0f, -10.0f };
    const Vector3 kDefaultLightDir = { 0.0f, -1.0f, 1.0f };
    const Vector3 kDefaultNormalZ = { 0.0f, 0.0f, 1.0f };
}

GameScene::GameScene() {}

GameScene::~GameScene() {}

void GameScene::Initialize(IrufemiEngine* engine) {
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
    directionalLight_->direction = kDefaultLightDir;
    directionalLight_->intensity = 1.0f;
}

// 更新
void GameScene::Update() {
    // =====

    // ↓ゲームの更新
    // =====

    // 独自のマウス更新は二重更新（デルタ値の消失）の原因になるため削除

    // プレイヤーの更新
    if (player_ && !debugMode_) {
        // ★プレイヤーにボスの座標を毎フレーム教える（機関銃とミサイルのオートエイム用）
        if (boss_) {
            Vector3 bossPos = boss_->GetGlobalTransform().translate;
            player_->SetTargetPosition(bossPos);

            // ★追加: ボスが画面（プレイヤーの前方）にいるかどうかの判定
            Vector3 playerPos = player_->GetTranslate();
            Vector3 toBoss = Math::Subtract(bossPos, playerPos);
            float len = Math::Length(toBoss);
            bool inScreen = false;

            if (len > kMathEpsilon) {
                // プレイヤーからボスへの方向ベクトル
                toBoss = { toBoss.x / len, toBoss.y / len, toBoss.z / len };

                // プレイヤーが向いている方向ベクトル
                float sinY = std::sin(player_->GetRotate().y);
                float cosY = std::cos(player_->GetRotate().y);
                Vector3 playerForward = { sinY, 0.0f, cosY };

                // 内積で前方にいるかを判定（0.0fより大きければ前方180度以内にいる）
                float dot = Math::Dot(toBoss, playerForward);
                if (dot > 0.0f) {
                    inScreen = true;
                }
            }

            // 判定結果をプレイヤーに渡す（これでミサイル倍増や機関銃の挙動が切り替わります）
            player_->SetIsTargetingEnemy(inScreen);
        }
        player_->Update();
    }

    if (boss_) {
        boss_->Update(player_.get());
    }

    // --- 当たり判定の実装 ---
    if (player_ && boss_) {
        // Playerの攻撃判定（近接）
        const AttackCollision& attackCol = player_->GetAttackCollision();
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
                if (player_->ApplyDamage(kDamageBeamToPlayer)) {
                    OutputDebugStringA(std::format("Player Hit by Beam: {} damage\n", kDamageBeamToPlayer).c_str());
                }
            }
        }

        // Player vs EnemyStompEffects
        if (boss_->GetStompEffects() && boss_->GetStompEffects()->IsActive()) {
            EnemyStompEffects* stomp = boss_->GetStompEffects();
            Sphere playerColliderSphere;
            playerColliderSphere.center = player_->GetCollider().center;
            playerColliderSphere.radius = player_->GetCollider().radius;

            // 1. 足元爆発の判定
            if (stomp->IsExplosionDamageActive() && !stomp->HasDealtExplosionDamage()) {
                Sphere explosionSphere;
                explosionSphere.center = stomp->GetBasePosition();
                explosionSphere.radius = stomp->GetExplosionRadius();
                if (Collision::IsSphereCollision(explosionSphere, playerColliderSphere)) {
                    if (player_->ApplyDamage(stomp->GetExplosionDamage())) {
                        stomp->SetDealtExplosionDamage(true);
                        OutputDebugStringA(std::format("Player Hit by Stomp Explosion: {} damage\n", stomp->GetExplosionDamage()).c_str());
                    }
                }
            }

            // 2. 噴き上がり爆発の判定
            if (stomp->IsFinalExplosionActive() && !stomp->HasDealtFinalDamage()) {
                if (Collision::IsOBBSphereCollision(stomp->GetFinalExplosionOBB(), playerColliderSphere)) {
                    if (player_->ApplyDamage(stomp->GetFinalExplosionDamage())) {
                        stomp->SetDealtFinalDamage(true);
                        OutputDebugStringA(std::format("Player Hit by Stomp Final: {} damage\n", stomp->GetFinalExplosionDamage()).c_str());
                    }
                }
            }
        }

        // Player vs Enemy Parts (Any active parts)
        auto checkPlayerHitPart = [&](auto* part) {
            if (!part || part->IsCompletelyDead())
                return;

            Sphere playerColliderSphere;
            playerColliderSphere.center = player_->GetCollider().center;
            playerColliderSphere.radius = player_->GetCollider().radius;

            if (Collision::IsOBBSphereCollision(part->GetOBB(), playerColliderSphere)) {
                if (player_->ApplyDamage(kDamagePartToPlayer)) {
                    OutputDebugStringA(std::format("Player Hit by Enemy Part: {} damage\n", kDamagePartToPlayer).c_str());
                }
            }
            };
        for (int i = 0; i < kEnemyBodyPartsCount; ++i) checkPlayerHitPart(boss_->GetBody(i));
        checkPlayerHitPart(boss_->GetHeadLeft());
        checkPlayerHitPart(boss_->GetHeadMid());
        checkPlayerHitPart(boss_->GetHeadRight());

        auto checkAndDamage = [&](auto* part) {
            if (!part || part->GetHP() <= 0 || part->IsBlownAway())
                return;

            // 1. 近接攻撃の判定
            if (attackCol.isActive &&
                Collision::IsOBBSphereCollision(part->GetOBB(), attackSphere)) {
                if (part->ApplyDamage(kDamageMeleeToEnemy)) {
                    OutputDebugStringA(std::format("Enemy Part Hit by Player Melee: {} damage\n", kDamageMeleeToEnemy).c_str());
                }
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
                attackOBB.orientations[0] = { 1, 0, 0 };
                attackOBB.orientations[1] = { 0, 1, 0 };
                attackOBB.orientations[2] = { 0, 0, 1 };
                attackOBB.size = { attackCol.radius * kMeleeEffectSizeMultiplier,
                                  attackCol.radius * kMeleeEffectSizeMultiplier,
                                  attackCol.radius * kMeleeEffectSizeMultiplier };

                // 攻撃方向を考慮した初速ではじけさせる
                Vector3 attackDir = Math::Normalize(
                    Math::Subtract(part->GetTransform().translate, playerPos));
                part->ScatterAt(Math::Multiply(kMeleeScatterSpeedMultiplier, attackDir), attackOBB);
            }

            // 2. マシンガンの弾の判定
            MachineGunBullet* bullets = player_->GetMachineGunBullets();
            for (int j = 0; j < Player::GetMaxMachineGunBullets(); ++j) {
                if (!bullets[j].isActive)
                    continue;
                Sphere bulletSphere;
                bulletSphere.center = bullets[j].position;
                bulletSphere.radius = kMachineGunBulletRadius; // 余裕を持たせた半径
                if (Collision::IsOBBSphereCollision(part->GetOBB(), bulletSphere)) {
                    bullets[j].isActive = false; // 弾丸消滅
                    if (part->ApplyDamage(kDamageMachineGunToEnemy)) { // マシンガンのダメージ
                        OutputDebugStringA(std::format("Enemy Part Hit by MachineGun: {} damage\n", kDamageMachineGunToEnemy).c_str());
                    }
                    if (part->GetHP() <= 0) {
                        Vector3 attackDir = Math::Normalize(bullets[j].velocity);
                        part->OnDestroyed(attackDir,
                            EnemyParameters::GetInstance()->GetBlowSpeed());
                    }
                }
            }

            // 3. ミサイルの判定
            MissileData* missiles = player_->GetMissiles();
            for (int k = 0; k < Player::GetMaxMissiles(); ++k) {
                if (!missiles[k].isActive)
                    continue;
                Sphere missileSphere;
                missileSphere.center = missiles[k].position;
                missileSphere.radius = kMissileRadius; // ミサイルの当たり判定を大きめに
                if (Collision::IsOBBSphereCollision(part->GetOBB(), missileSphere)) {
                    missiles[k].isActive = false;    // ミサイル消滅
                    if (part->ApplyDamage(kDamageMissileToEnemy)) { // ミサイルのダメージ
                        OutputDebugStringA(std::format("Enemy Part Hit by Missile: {} damage\n", kDamageMissileToEnemy).c_str());
                    }
                    if (part->GetHP() <= 0) {
                        Vector3 attackDir = Math::Normalize(missiles[k].velocity);
                        part->OnDestroyed(attackDir,
                            EnemyParameters::GetInstance()->GetBlowSpeed());
                    }
                }
            }
            };

        // 各部位に対して判定
        for (int i = 0; i < kEnemyBodyPartsCount; ++i) {
            checkAndDamage(boss_->GetBody(i));
        }
        checkAndDamage(boss_->GetHeadLeft());
        checkAndDamage(boss_->GetHeadMid());
        checkAndDamage(boss_->GetHeadRight());

        // 吹き飛んだ部位(projectile) vs 生存している各部位(target) の判定
        auto checkProjectileHitPart = [&](auto* projectile) {
            if (!projectile || !projectile->IsBlownAway() ||
                projectile->IsCompletelyDead())
                return;

            // 毎回のターゲット（全部位）との判定ループ内で何度もGetOBB()を呼ばないようにキャッシュする
            OBB projectileOBB = projectile->GetOBB();

            auto checkTarget = [&](auto* target) {
                // targetはまだ生きているか（吹き飛んでいないか）
                if (!target || target->GetHP() <= 0 || target->IsBlownAway())
                    return;

                // OBB同士の判定(部位 vs 部位)
                if (Collision::IsOBBCollision(projectileOBB, target->GetOBB())) {
                    Vector3 vel = projectile->GetBlowVelocity();
                    Vector3 diff = Math::Subtract(projectile->GetTransform().translate,
                        target->GetTransform().translate);

                    // XZ平面での法線を計算
                    Vector3 normal = { diff.x, 0.0f, diff.z };
                    float normalLen = Math::Length(normal);
                    if (normalLen > kMathEpsilon) {
                        normal = { normal.x / normalLen, 0.0f, normal.z / normalLen };
                    } else {
                        normal = kDefaultNormalZ;
                    }

                    // 部位の速度と、ターゲットから部位への方向ベクトルの内積
                    float dot = Math::Dot(vel, normal);

                    // dot < 0.0f
                    // なら、部位がターゲットに向かって飛んできている（めり込み・連続ヒット防止）
                    if (dot < 0.0f) {
                        // 当たった部位のみにダメージを与える
                        if (target->ApplyDamage(kDamageProjectilePartToEnemy)) {
                            OutputDebugStringA(std::format("Enemy Part Hit by Flying Projectile Part: {} damage\n", kDamageProjectilePartToEnemy).c_str());
                        }

                        // 反射ベクトル: R = V - 2(V・N)N
                        Vector3 reflect =
                            Math::Subtract(vel, Math::Multiply(2.0f * dot, normal));
                        reflect.y = 0.0f; // Y方向には飛ばないように固定

                        // 速度を反転・反射させる
                        projectile->SetBlowVelocity(reflect);

                        // ターゲットのHPが尽きた場合
                        if (target->GetHP() <= 0) {
                            float velLen = Math::Length(vel);
                            Vector3 attackDir = kDefaultNormalZ;
                            if (velLen > kMathEpsilon) {
                                attackDir = { vel.x / velLen, 0.0f, vel.z / velLen };
                            }
                            target->OnDestroyed(
                                attackDir, EnemyParameters::GetInstance()->GetBlowSpeed());
                        }

                        // 衝突エフェクト（部位同士の衝突）
                        // ターゲットのOBBをそのままはじける領域として指定
                        target->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, vel), target->GetOBB());
                        projectile->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, reflect),
                            projectile->GetOBB());
                    }
                }
                };

            auto checkTargetProjectile = [&](auto* target) {
                if (!target || (void*)target == (void*)projectile || !target->IsBlownAway() || target->IsCompletelyDead())
                    return;

                if (Collision::IsOBBCollision(projectileOBB, target->GetOBB())) {
                    Vector3 vel1 = projectile->GetBlowVelocity();
                    Vector3 vel2 = target->GetBlowVelocity();

                    Vector3 diff = Math::Subtract(projectile->GetTransform().translate, target->GetTransform().translate);
                    Vector3 normal = { diff.x, 0.0f, diff.z };
                    float normalLen = Math::Length(normal);
                    if (normalLen > kMathEpsilon) {
                        normal = { normal.x / normalLen, 0.0f, normal.z / normalLen };
                    } else {
                        normal = kDefaultNormalZ;
                    }

                    // 相対速度
                    Vector3 relVel = Math::Subtract(vel1, vel2);
                    float dot = Math::Dot(relVel, normal);

                    if (dot < 0.0f) {
                        Vector3 change = Math::Multiply(dot, normal);
                        Vector3 bounce1 = Math::Subtract(vel1, change);
                        Vector3 bounce2 = Math::Add(vel2, change);

                        bounce1.y = 0.0f;
                        bounce2.y = 0.0f;

                        projectile->SetBlowVelocity(bounce1);
                        target->SetBlowVelocity(bounce2);

                        projectile->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, bounce1), projectile->GetOBB());
                        target->ScatterAt(Math::Multiply(kCollisionScatterMultiplier, bounce2), target->GetOBB());
                    }
                }
                };

            for (int i = 0; i < kEnemyBodyPartsCount; ++i) {
                checkTarget(boss_->GetBody(i));
                checkTargetProjectile(boss_->GetBody(i));
            }
            checkTarget(boss_->GetHeadLeft());
            checkTargetProjectile(boss_->GetHeadLeft());
            checkTarget(boss_->GetHeadMid());
            checkTargetProjectile(boss_->GetHeadMid());
            checkTarget(boss_->GetHeadRight());
            checkTargetProjectile(boss_->GetHeadRight());
            };

        for (int i = 0; i < kEnemyBodyPartsCount; ++i)
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

    // --- カメラとフレームデータの更新 ---
    UpdateCameraAndFrameData();

    // enemyが死んだときクリアシーンに遷移する
    if (boss_ && boss_->IsDead()) {
        if (engine_ && engine_->GetSceneManager()) {
            engine_->GetSceneManager()->Request("Clear"); // クリアシーンへの遷移
        }
    }

    // ★追加: プレイヤーの死亡演出が終了したときゲームオーバーシーンに遷移する
    if (player_ && player_->IsDeathAnimationFinished()) {
        if (engine_ && engine_->GetSceneManager()) {
            engine_->GetSceneManager()->Request("GameOver"); // "GameOver" は実際のシーン名に合わせてください
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

void GameScene::PauseUpdate() {
    UpdateCameraAndFrameData();
}
void GameScene::PauseDraw() {}

void GameScene::DrawDebugTab() {
#ifdef USE_IMGUI
    if (camera_) {
        if (ImGui::BeginTabItem("Main Camera")) {
            ImGui::Checkbox("Debug Camera Mode", &debugMode_);
            if (debugMode_ && debugCamera_) {
                if (ImGui::Button("Top-Down")) debugCamera_->SetPreset(DebugCamera::Preset::TopDown, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Diagonal")) debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Front")) debugCamera_->SetPreset(DebugCamera::Preset::Front, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Snap to Current")) debugCamera_->SetPreset(DebugCamera::Preset::Current, *camera_);

                ImGui::Separator();
                ImGui::Text("Debug Camera Controls");
                // DebugCameraの内部Cameraの設定を表示
                debugCamera_->GetCamera().DrawDebugContents();
                float dist = debugCamera_->GetDistance();
                if (ImGui::DragFloat("Orbit Distance", &dist, kDebugCameraDragSpeed, kDebugCameraDistMin, kDebugCameraDistMax)) {
                    debugCamera_->SetDistance(dist);
                }
            } else {
                camera_->DrawDebugContents();
            }
            ImGui::EndTabItem();
        }
    }
    DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);
    if (ImGui::BeginTabItem("InGame")) {

        ImGui::Checkbox("Debug Camera", &debugMode_);
        ImGui::EndTabItem();
    }
#endif
}

void GameScene::UpdateCameraAndFrameData() {
    // --- デバッグカメラのトグル ('P' キー) ---
    if (PressedDIK(kKeyDebugCameraToggle)) {
        debugMode_ = !debugMode_;
        if (debugMode_) {
            if (isFirstDebug_) {
                debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *camera_);
                isFirstDebug_ = false;
            }
        } else {
            // デバッグカメラ OFF 時、通常カメラの状態を一度強制更新して復元を確実にする
            if (player_) player_->Update();
            camera_->Update();
        }
    }

    // --- カメラの更新 ---
    if (debugMode_) {
        // デバッグカメラを更新
        debugCamera_->Update();
        // デバッグカメラの計算結果をメインカメラに上書きする
        const Camera& dbgCam = debugCamera_->GetCamera();
        camera_->SetViewMatrix(dbgCam.GetViewMatrix());
        camera_->SetTranslate(dbgCam.GetTranslate());
        camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
    } else {
        // 通常カメラの更新（プレイヤーのカメラ位置を反映する）
        camera_->Update();
    }

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = camera_->GetViewMatrix();
    cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = camera_->GetTranslate();

    // ライトリストの構築 (スマートポインタから生ポインタへ)
    std::vector<PointLight*> pLights;
    for (auto& pl : pointLights_) pLights.push_back(pl.get());
    std::vector<SpotLight*> sLights;
    for (auto& sl : spotLights_) sLights.push_back(sl.get());
    std::vector<AreaLight*> aLights;
    for (auto& al : areaLights_) aLights.push_back(al.get());

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);

    // 環境マップ（白色CubeMap）を設定
    engine_->GetDrawManager()->SetEnvironmentMap(engine_->GetTextureManager()->GetWhiteCubeMapHandle());
}

