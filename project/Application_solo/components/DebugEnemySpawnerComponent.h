#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Utility/ObjectPool.h"
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

private:
    void SpawnEnemy(const Irufemi::Vector3& position);

    int maxEnemies_ = 50;
    std::unique_ptr<ObjectPool<GameObject>> enemyPool_;
    std::unordered_map<GameObject*, ObjectPool<GameObject>::Handle> activeEnemyHandles_;
};
