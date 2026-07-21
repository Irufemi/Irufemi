#include "EnvironmentManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/Utility/Log.h"

void EnvironmentManagerComponent::Initialize() {
    LoadSpawnList();
}

void EnvironmentManagerComponent::Start() {
    SpawnAll();
}

void EnvironmentManagerComponent::Update() {
    // Dynamic spawn based on distance could be implemented here
}

void EnvironmentManagerComponent::LoadSpawnList() {
    spawnList_.clear();
    
    // Hardcoded test layout based on TripoStudio generated models
    
    // 1. Pillars along the sides to give a sense of speed
    for (int i = 0; i < 10; ++i) {
        float zPos = i * 50.0f;
        // Left pillar
        spawnList_.push_back({"resources/prefabs/Env_Pillar.json", {-20.0f, 0.0f, zPos}, {0,0,0}, {4,4,4}});
        // Right pillar
        spawnList_.push_back({"resources/prefabs/Env_Pillar.json", {20.0f, 0.0f, zPos}, {0,0,0}, {4,4,4}});
    }

    // 2. Arches that the player flies through
    for (int i = 0; i < 5; ++i) {
        float zPos = 100.0f + i * 100.0f;
        spawnList_.push_back({"resources/prefabs/Env_Arch.json", {0.0f, -5.0f, zPos}, {0,0,0}, {4,4,4}});
    }

    // 3. Walls that block half the path to force dodging
    spawnList_.push_back({"resources/prefabs/Env_Wall.json", {-10.0f, 0.0f, 150.0f}, {0,0,0}, {4,4,4}});
    spawnList_.push_back({"resources/prefabs/Env_Wall.json", {10.0f, 0.0f, 250.0f}, {0,0,0}, {4,4,4}});
    spawnList_.push_back({"resources/prefabs/Env_Wall.json", {0.0f, 5.0f, 350.0f}, {0,0,0}, {4,4,4}});
}

void EnvironmentManagerComponent::SpawnAll() {
    if (!gameObject_) {
        return;
    }
    if (!gameObject_->GetScene()) {
        return;
    }

    for (const auto& data : spawnList_) {
        auto obj = gameObject_->GetScene()->InstantiatePrefab(data.prefabPath, data.position);
        if (obj) {
            gameObject_->AddChild(obj);
            if (auto transform = obj->GetComponent<TransformComponent>()) {
                transform->SetRotation(data.rotation);
                transform->SetScale(data.scale);
            }
            spawnedObjects_.push_back(obj);
        }
    }
}
