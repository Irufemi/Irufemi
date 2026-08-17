#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"
#include "Core/Utility/ObjectPool.h"
#include <memory>
#include <unordered_map>

class DebugEnemySpawnerComponent : public Component {
public:
    DebugEnemySpawnerComponent() = default;
    ~DebugEnemySpawnerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override {}
    std::string GetComponentName() const override { return "DebugEnemySpawnerComponent"; }

    void SpawnEnemy(const Irufemi::Vector3& position, const Irufemi::Vector3& rotation);

    int maxEnemies_ = 50;
    std::unique_ptr<ObjectPool<GameObject>> enemyPool_;
    std::unordered_map<GameObject*, ObjectPool<GameObject>::Handle> activeEnemyHandles_;
};
