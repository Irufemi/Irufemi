#include "EnvironmentManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/Utility/Log.h"

void EnvironmentManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();

    if (batchCollisionSettings_.empty()) {
        batchCollisionSettings_.push_back({
            "Env_Pillar", 
            Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, -1.0f, -1.0f),
            Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f),
            0, 0
        });
        batchCollisionSettings_.push_back({
            "Env_Arch", 
            Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, -1.0f, -1.0f),
            Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f),
            0, 0
        });
        batchCollisionSettings_.push_back({
            "Env_Wall", 
            Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, -1.0f, -1.0f),
            Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f),
            0, 0
        });
    }

    RegisterHeader("Batch Collisions");
    for (auto& setting : batchCollisionSettings_) {
        std::string name = setting.prefabPath;
        RegisterProperty("ColSize_" + name, &setting.collisionSize);
        RegisterProperty("ColOffset_" + name, &setting.collisionOffset);
        // Type_ is kept for serialization compatibility, but no longer modifies Y position.
        RegisterEnum("Type_" + name, &setting.placementType, {"Building", "Floating"});
    }
}

void EnvironmentManagerComponent::Initialize() {
}

void EnvironmentManagerComponent::Start() {
    if (!gameObject_) return;

    // 子オブジェクトとして配置されている環境オブジェクトを検索して追跡リストに登録
    const auto& children = gameObject_->GetChildren();
    for (const auto& child : children) {
        if (!child) continue;

        for (auto& setting : batchCollisionSettings_) {
            if (child->GetName().find(setting.prefabPath) != std::string::npos) {
                // 初回のみプレハブからデフォルトのサイズを取得する
                if (setting.collisionSize.x < 0.0f) {
                    if (auto obb = child->GetComponent<OBBColliderComponent>()) {
                        setting.collisionSize = obb->GetLocalSize();
                        setting.previousSize = setting.collisionSize;
                        setting.collisionOffset = obb->GetLocalOffset();
                        setting.previousOffset = setting.collisionOffset;
                    }
                }

                Vector3 origPos, origRot, origScale;
                if (auto transform = child->GetComponent<TransformComponent>()) {
                    origPos = transform->GetPosition();
                    origRot = transform->GetRotation();
                    origScale = transform->GetScale();
                }
                spawnedObjects_.push_back({child, setting.prefabPath, origPos, origRot, origScale});
                break;
            }
        }
    }
    
    // 最初のバッチ設定を適用
    for (const auto& info : spawnedObjects_) {
        if (auto obj = info.obj.lock()) {
            if (auto obb = obj->GetComponent<OBBColliderComponent>()) {
                for (const auto& setting : batchCollisionSettings_) {
                    if (setting.prefabPath == info.prefabPath) {
                        obb->SetLocalSize(setting.collisionSize);
                        obb->SetLocalOffset(setting.collisionOffset);
                        break;
                    }
                }
            }
        }
    }
}

void EnvironmentManagerComponent::Update() {
    bool anyChanged = false;
    for (auto& setting : batchCollisionSettings_) {
        if (setting.collisionSize != setting.previousSize ||
            setting.collisionOffset != setting.previousOffset) {
            setting.previousSize = setting.collisionSize;
            setting.previousOffset = setting.collisionOffset;
            anyChanged = true;
        }
        // placementType is tracked but unused since manual placement defines Transform
        if (setting.placementType != setting.previousPlacementType) {
            setting.previousPlacementType = setting.placementType;
        }
    }

    if (anyChanged) {
        for (const auto& info : spawnedObjects_) {
            if (auto obj = info.obj.lock()) {
                if (auto obb = obj->GetComponent<OBBColliderComponent>()) {
                    for (const auto& setting : batchCollisionSettings_) {
                        if (setting.prefabPath == info.prefabPath) {
                            obb->SetLocalSize(setting.collisionSize);
                            obb->SetLocalOffset(setting.collisionOffset);
                            break;
                        }
                    }
                }
            }
        }
    }
}
