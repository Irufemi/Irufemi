#include "Combat/DebugEnemySpawnerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Core/Math/Random/Random.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Player/TargetableComponent.h"

// AAAタイトルのアプローチ (Data-Oriented Design & Instancing)
// 個々の敵オブジェクトにMeshRendererを持たせるのではなく、Spawnerが一括でModelBatchRendererComponentを管理します。
// これにより、数千体の敵を描画する際でもドローコールが1回（Instancing）に削減され、
// CPUとGPUのオーバーヘッドが劇的に改善されます（Unreal EngineのHISMやUnityのDOTSに近いアーキテクチャ）。

DebugEnemySpawnerComponent::~DebugEnemySpawnerComponent() {
    if (enemyPool_) {
        enemyPool_->ForEach([](const std::shared_ptr<GameObject>& enemy) {
            if (enemy) {
                if (auto enemyComp = enemy->GetComponent<RailShooterEnemyComponent>()) {
                    enemyComp->SetOnDeathCallback(nullptr);
                }
            }
        });
    }
}

void DebugEnemySpawnerComponent::Initialize() {}

void DebugEnemySpawnerComponent::OnRegisterProperties() {
    RegisterProperty("Enemy Model Path", &enemyModelPath_);
}

void DebugEnemySpawnerComponent::Start() {
    batchRenderer_ = gameObject_->AddComponent<ModelBatchRendererComponent>();
    batchRenderer_->LoadModel(enemyModelPath_);

    auto scene = gameObject_->GetScene();
    if (!scene) {
        return;
    }

    auto weakObj = gameObject_->weak_from_this();

    enemyPool_ = std::make_unique<ObjectPool<GameObject>>(maxEnemies_, [weakObj]() {
        auto enemy = std::make_shared<GameObject>("DebugEnemy");
        enemy->SetIsSerializable(false); // セーブデータ（JSON）への混入を防止

        // スポナーの子オブジェクトとして登録しライフサイクルを同期
        if (auto spawnerObj = weakObj.lock()) {
            spawnerObj->AddChild(enemy);
        }

        auto transform = enemy->GetTransform();
        transform->SetScale({1.2f, 1.2f, 1.2f});

        auto enemyComp = enemy->AddComponent<RailShooterEnemyComponent>();
        enemyComp->SetOnDeathCallback([weakObj](GameObject* deadObj) {
            deadObj->SetIsActive(false);
            // スポナーの生存確認（ダングリングポインタによるクラッシュを防止）
            if (auto spawnerObj = weakObj.lock()) {
                if (auto spawner = spawnerObj->GetComponent<DebugEnemySpawnerComponent>()) {
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

        enemy->SetIsActive(false);
        return enemy;
    });
}

void DebugEnemySpawnerComponent::Update() {
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

    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) {
        return;
    }

    // '2'キーで敵をスポーン
    if (input->IsKeyPressed('2')) {
        Irufemi::Vector3 spawnPos = {0.0f, 0.0f, 50.0f};
        Irufemi::Vector3 spawnRot = {0.0f, 3.14159f, 0.0f};

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
                    spawnRot.y += 3.14159f;
                }
            }
        }

        SpawnEnemy(spawnPos, spawnRot);
    }
}

GameObject* DebugEnemySpawnerComponent::SpawnEnemy(const Irufemi::Vector3& position, const Irufemi::Vector3& rotation) {
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
        }

        if (auto enemyComp = enemy->GetComponent<RailShooterEnemyComponent>()) {
            // プールから復帰した際に必要な初期化（HPリセット等）を呼ぶ想定
            enemyComp->Initialize();
        }

        enemy->SetIsActive(true);
        return enemy.get();
    }

    return nullptr;
}
