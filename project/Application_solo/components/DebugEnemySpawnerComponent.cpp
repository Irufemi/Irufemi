#include "DebugEnemySpawnerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "RailShooterEnemyComponent.h"
#include "Engine/Core/Math/Random/Random.h"

void DebugEnemySpawnerComponent::Initialize() {
}

void DebugEnemySpawnerComponent::Start() {
    auto scene = gameObject_->GetScene();
    if (!scene) return;

    enemyPool_ = std::make_unique<ObjectPool<GameObject>>(maxEnemies_, [this, scene]() {
        auto enemy = std::make_shared<GameObject>("DebugEnemy");
        // scene->AddGameObject(enemy);
        
        auto transform = enemy->GetTransform();
        transform->SetScale({1.2f, 1.2f, 1.2f});

        enemy->AddComponent<PrimitiveRendererComponent>();
        auto enemyComp = enemy->AddComponent<RailShooterEnemyComponent>();

        // プール運用のため、エネミー死亡時は Destroy ではなくプールへ返却する
        enemyComp->SetOnDeathCallback([this, scene](GameObject* deadObj) {
            deadObj->SetIsActive(false);
            if (enemyPool_) {
                auto it = activeEnemyHandles_.find(deadObj);
                if (it != activeEnemyHandles_.end()) {
                    enemyPool_->Release(it->second);
                    activeEnemyHandles_.erase(it);
                    if (scene) scene->RemoveGameObject(deadObj->shared_from_this());
                }
            }
        });

        enemy->SetIsActive(false);
        return enemy;
    });
}

void DebugEnemySpawnerComponent::Update() {
    auto input = BaseModel::GetIrufemiEngine()->GetInputManager();
    if (!input) return;

    // '2'キーで敵をスポーン
    if (input->IsKeyPressed('2')) {
        Irufemi::Vector3 spawnPos = {0.0f, 0.0f, 50.0f};

        auto scene = gameObject_->GetScene();
        if (scene) {
            auto playerObj = scene->FindGameObject("Player");
            if (playerObj) {
                if (auto transform = playerObj->GetComponent<TransformComponent>()) {
                    // プレイヤーの現在位置から Z軸前方に 50m、XYはランダムに散らす
                    spawnPos = transform->GetPosition();
                    spawnPos.z += 50.0f;
                    spawnPos.x += Irufemi::Random::GeneratorFloat(-10.0f, 10.0f);
                    spawnPos.y += Irufemi::Random::GeneratorFloat(-5.0f, 5.0f);
                }
            }
        }

        SpawnEnemy(spawnPos);
    }
}

void DebugEnemySpawnerComponent::SpawnEnemy(const Irufemi::Vector3& position) {
    if (!enemyPool_) return;

    auto handle = enemyPool_->Acquire();
    if (!handle.IsValid()) return;
    
    auto enemy = enemyPool_->Resolve(handle);
    if (enemy) {
        // マップに登録
        activeEnemyHandles_[enemy.get()] = handle;
        
        if (auto scene = gameObject_->GetScene()) {
            scene->AddGameObject(enemy);
        }

        if (auto transform = enemy->GetComponent<TransformComponent>()) {
            transform->SetWorldPosition(position);
            transform->SetWorldRotation({0.0f, 3.14159f, 0.0f});
        }
        
        if (auto enemyComp = enemy->GetComponent<RailShooterEnemyComponent>()) {
            // プールから復帰した際に必要な初期化（HPリセット等）を呼ぶ想定
            enemyComp->Initialize(); 
        }

        enemy->SetIsActive(true);
    }
}
