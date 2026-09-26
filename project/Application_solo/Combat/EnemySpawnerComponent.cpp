#include "Combat/EnemySpawnerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Core/Math/Random/Random.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Player/TargetableComponent.h"
#include "Framework/Prefab/PrefabUtility.h"
#include "Core/Math/MathFunction.h"
#include "Environment/DebrisManagerComponent.h"
#include "Effects/EffectManagerComponent.h"
#include "Renderer/System/VoxelParticle/VoxelParticleManager.h"

// AAAタイトルのアプローチ (Data-Oriented Design & Instancing)
// 個々の敵オブジェクトにMeshRendererを持たせるのではなく、Spawnerが一括でModelBatchRendererComponentを管理します。
// これにより、数千体の敵を描画する際でもドローコールが1回（Instancing）に削減され、
// CPUとGPUのオーバーヘッドが劇的に改善されます（Unreal EngineのHISMやUnityのDOTSに近いアーキテクチャ）。

EnemySpawnerComponent::~EnemySpawnerComponent() {
    if (enemyPool_) {
        enemyPool_->ForEach([](const std::shared_ptr<GameObject>& enemy) {
            if (enemy) {
                if (auto enemyComp = enemy->GetComponent<RailShooterEnemyComponent>()) {
                    enemyComp->SetOnDeathCallback(nullptr);
                    enemyComp->SetOnDespawnListener(nullptr);
                }
            }
        });
    }
}

void EnemySpawnerComponent::Initialize() {}

void EnemySpawnerComponent::OnRegisterProperties() {
    RegisterProperty("Enemy Model Path", &enemyModelPath_);
    RegisterProperty("Enemy Prefab Path", &enemyPrefabPath_);
    RegisterProperty("Base Enemy Scale", &baseEnemyScale_);
    RegisterProperty("Base Collider Radius", &baseColliderRadius_);
}

void EnemySpawnerComponent::Start() {
    // プレハブ（Archetype）からモデル・基本スケール・当たり判定半径を自動解決
    auto metrics = PrefabUtility::ExtractMetrics(enemyPrefabPath_);
    if (!metrics.modelPath.empty()) {
        enemyModelPath_ = metrics.modelPath;
    }
    baseEnemyScale_ = metrics.baseScale;
    if (metrics.hasSphereCollider) {
        baseColliderRadius_ = (std::max)(metrics.colliderRadius, 1.6f);
    } else {
        baseColliderRadius_ = 1.8f;
    }

    batchRenderer_ = gameObject_->AddComponent<ModelBatchRendererComponent>();
    batchRenderer_->LoadModel(enemyModelPath_);

    auto scene = gameObject_->GetScene();
    if (!scene) {
        return;
    }

    auto weakObj = gameObject_->weak_from_this();

    enemyPool_ = std::make_unique<ObjectPool<GameObject>>(maxEnemies_, [this, weakObj]() {
        std::shared_ptr<GameObject> enemy = nullptr;
        if (auto spawnerObj = weakObj.lock()) {
            enemy = spawnerObj->Instantiate(enemyPrefabPath_);
        }
        if (!enemy) {
            enemy = std::make_shared<GameObject>("DebugEnemy");
            enemy->AddComponent<RailShooterEnemyComponent>();
        }

        enemy->SetIsSerializable(false); // セーブデータ（JSON）への混入を防止

        // スポナーの子オブジェクトとして登録しライフサイクルを同期
        if (auto spawnerObj = weakObj.lock()) {
            spawnerObj->AddChild(enemy);
        }

        // プレハブ単体プレビュー用レンダラーがあれば削除し、SpawnerのBatchRenderer（Instancing）で一括描画
        if (auto meshRenderer = enemy->GetComponent<MeshRendererComponent>()) {
            enemy->RemoveComponent(meshRenderer);
        }

        auto transform = enemy->GetTransform();
        if (transform) {
            transform->SetScale(baseEnemyScale_);
        }

        if (auto collider = enemy->GetComponent<SphereColliderComponent>()) {
            collider->SetLocalRadius(baseColliderRadius_);
        }

        if (auto enemyComp = enemy->GetComponent<RailShooterEnemyComponent>()) {
            enemyComp->SetOnDespawnListener([weakObj](GameObject* deadObj, DespawnReason reason) {
                if (!deadObj) {
                    return;
                }

                // プレイヤー撃破（または自爆体当たり）時のみ、残骸ガレキドロップ＆Voxel破砕演出を実行
                if (reason == DespawnReason::KilledByPlayer || reason == DespawnReason::CollisionSuicide) {
                    auto deadTrans = deadObj->GetComponent<TransformComponent>();
                    Irufemi::Vector3 deadPos =
                        deadTrans ? deadTrans->GetWorldPosition() : Irufemi::Vector3{0.0f, 0.0f, 0.0f};
                    Irufemi::Vector3 currentScale =
                        deadTrans ? deadTrans->GetScale() : Irufemi::Vector3{1.0f, 1.0f, 1.0f};

                    auto scene = deadObj->GetScene();
                    if (scene) {
                        // 1. プレイヤー位置の取得
                        Irufemi::Vector3 playerPos = deadPos;
                        auto playerObj = scene->FindGameObject("PlayerCart");
                        if (!playerObj) {
                            playerObj = scene->FindGameObject("Player");
                        }
                        if (playerObj && playerObj->GetTransform()) {
                            playerPos = playerObj->GetTransform()->GetWorldPosition();
                        }

                        // 2. スケール連動のドロップ個数決定（中型機は大量ドロップボーナス）
                        float scaleMult = 1.0f;
                        if (auto ec = deadObj->GetComponent<RailShooterEnemyComponent>()) {
                            scaleMult = ec->GetScaleMultiplier();
                        }
                        int dropCount = (scaleMult >= 1.3f) ? 4 : 2;

                        // 3. 自機手前方向への物理バーストドロップ（通常ガレキのランダム散乱）
                        if (auto debrisMgrObj = scene->FindGameObject("DebrisManager")) {
                            if (auto debrisMgr = debrisMgrObj->GetComponent<DebrisManagerComponent>()) {
                                debrisMgr->SpawnDebrisBurst(deadPos, playerPos, dropCount, 3.5f, 12.0f);
                            }
                        }

                        // 4. 敵機体のVoxelメッシュ破砕演出
                        if (auto engine = scene->GetEngine()) {
                            if (auto voxelMgr = engine->GetVoxelParticleManager()) {
                                VoxelEmitter p{};
                                p.particleType = 5; // DebrisExplosive
                                p.lifeTime = 1.2f;
                                p.gravity = 4.0f;
                                p.dispersion = 14.0f;
                                p.scale = {0.5f, 0.5f, 0.5f};
                                p.startColor = {1.8f, 1.3f, 0.9f, 1.0f}; // 激しい火花・金属破砕光
                                p.endColor = {0.1f, 0.1f, 0.1f, 1.0f};
                                voxelMgr->PlayExplosion("Enemy_GravityGolem_A/SM_Enemy_GravityGolem_A.obj", deadPos,
                                                        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, currentScale, p,
                                                        {2, 2, 2});
                            }
                        }

                        // 5. 撃破爆発エフェクトの再生
                        if (auto effectMgr = EffectManagerComponent::GetInstance()) {
                            effectMgr->PlayEffect("debris_dust_effect", deadPos);
                        }
                    }
                }

                // プールへの返却（画面外離脱・タイムアウト時も安全にここへ合流して再利用）
                deadObj->SetIsActive(false);
                if (auto spawnerObj = weakObj.lock()) {
                    if (auto spawner = spawnerObj->GetComponent<EnemySpawnerComponent>()) {
                        if (spawner->enemyPool_) {
                            auto it = spawner->activeEnemyHandles_.find(deadObj);
                            if (it != spawner->activeEnemyHandles_.end()) {
                                spawner->enemyPool_->Release(it->second);
                                spawner->activeEnemyHandles_.erase(it);
                            }
                        }
                    }
                }
            });
        }

        enemy->SetIsActive(false);
        return enemy;
    });
}

void EnemySpawnerComponent::Update() {
    if (batchRenderer_) {
        // 毎フレーム、バッチレンダラーのインスタンス（描画キュー）をクリアします。
        batchRenderer_->ClearInstances();

        // アクティブなすべての敵のトランスフォームを収集し、一括登録します（Instancing描画）。
        for (const auto& pair : activeEnemyHandles_) {
            GameObject* enemyObj = pair.first;
            if (enemyObj && enemyObj->GetIsActive()) {
                batchRenderer_->AddInstanceWorld(enemyObj->GetTransform()->GetWorldMatrix());
            }
        }
    }

    auto engine = GetEngine();
    auto input = engine ? engine->GetInputManager() : nullptr;
    if (!input) {
        return;
    }

    // '2'キーで敵をスポーン
    if (input->IsKeyPressed('2')) {
        Irufemi::Vector3 spawnPos = {0.0f, 0.0f, 50.0f};
        Irufemi::Vector3 spawnRot = {0.0f, Irufemi::Math::PI, 0.0f};

        auto scene = gameObject_->GetScene();
        if (scene) {
            auto playerObj = scene->FindGameObject("Player");
            if (playerObj) {
                if (auto transform = playerObj->GetComponent<TransformComponent>()) {
                    // プレイヤーのワールド前方へ50m
                    spawnPos = transform->GetWorldPosition();
                    auto forward = transform->GetWorldForward();
                    spawnPos.x += forward.x * 50.0f;
                    spawnPos.y += forward.y * 50.0f;
                    spawnPos.z += forward.z * 50.0f;

                    // プレイヤーの右方向と上方向に少し散らす
                    auto right = transform->GetWorldRight();
                    auto up = transform->GetWorldUp();

                    float randX = Irufemi::Random::GeneratorFloat(-10.0f, 10.0f);
                    float randY = Irufemi::Random::GeneratorFloat(-5.0f, 5.0f);

                    spawnPos.x += right.x * randX + up.x * randY;
                    spawnPos.y += right.y * randX + up.y * randY;
                    spawnPos.z += right.z * randX + up.z * randY;

                    // プレイヤーと向かい合うように回転を設定（180度反転）
                    spawnRot = transform->GetWorldRotation();
                    spawnRot.y += Irufemi::Math::PI;
                }
            }
        }

        SpawnEnemy(spawnPos, spawnRot);
    }
}

GameObject* EnemySpawnerComponent::SpawnEnemy(const Irufemi::Vector3& position, const Irufemi::Vector3& rotation,
                                              float scaleMultiplier) {
    if (!enemyPool_) {
        return nullptr;
    }

    auto handle = enemyPool_->Acquire();
    if (!handle.IsValid()) {
        return nullptr;
    }

    auto enemy = enemyPool_->Resolve(handle);
    if (enemy) {
        activeEnemyHandles_[enemy.get()] = handle;

        if (auto transform = enemy->GetComponent<TransformComponent>()) {
            transform->SetWorldPosition(position);
            transform->SetWorldRotation(rotation);
            transform->SetScale(baseEnemyScale_ * scaleMultiplier);
        }

        if (auto collider = enemy->GetComponent<SphereColliderComponent>()) {
            collider->SetLocalRadius(baseColliderRadius_ * scaleMultiplier);
        }

        if (auto enemyComp = enemy->GetComponent<RailShooterEnemyComponent>()) {
            // プールから復帰した際に必要な初期化（HPリセット等）を呼ぶ
            enemyComp->Initialize();
            enemyComp->SetScaleMultiplier(scaleMultiplier);
        }

        enemy->SetIsActive(true);
        return enemy.get();
    }

    return nullptr;
}
