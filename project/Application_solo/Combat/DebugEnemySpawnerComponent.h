#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"
#include "Core/Utility/ObjectPool.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include <memory>
#include <unordered_map>

class DebugEnemySpawnerComponent : public Component {
public:
    DebugEnemySpawnerComponent() = default;
    ~DebugEnemySpawnerComponent() override;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override {
        return "DebugEnemySpawnerComponent";
    }

    GameObject* SpawnEnemy(const Irufemi::Vector3& position, const Irufemi::Vector3& rotation, float scaleMultiplier = 1.0f);

    int maxEnemies_ = 50;
    std::string enemyModelPath_ = "Enemy_GravityGolem_A/SM_Enemy_GravityGolem_A.obj";
    std::string enemyPrefabPath_ = "resources/prefabs/Enemy_GravityGolem.json";
    Irufemi::Vector3 baseEnemyScale_ = {1.2f, 1.2f, 1.2f};
    float baseColliderRadius_ = 2.0f;
    std::unique_ptr<ObjectPool<GameObject>> enemyPool_;
    std::unordered_map<GameObject*, ObjectPool<GameObject>::Handle> activeEnemyHandles_;
    std::shared_ptr<ModelBatchRendererComponent> batchRenderer_;
};
