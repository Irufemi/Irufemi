#include "EnvironmentManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/Utility/Log.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/EffectMaskComponent.h"

#include <sstream>

void EnvironmentManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();

    RegisterProperty("Target Prefabs", &targetPrefabNames_)
        .SetTooltip("Comma separated list of prefab names to manage (e.g. Env_Pillar,Env_Arch,Env_Wall)");

    // Split targetPrefabNames_ by comma
    std::vector<std::string> names;
    std::stringstream ss(targetPrefabNames_);
    std::string item;
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t\r\n"));
        item.erase(item.find_last_not_of(" \t\r\n") + 1);
        if (!item.empty()) {
            names.push_back(item);
        }
    }

    // Keep existing settings, add new ones, remove old ones
    std::list<BatchCollisionSetting> newSettings;
    for (const auto& name : names) {
        bool found = false;
        for (const auto& setting : batchCollisionSettings_) {
            if (setting.prefabPath == name) {
                newSettings.push_back(setting);
                found = true;
                break;
            }
        }
        if (!found) {
            newSettings.push_back({
                name, 
                Irufemi::Vector3(-1.0f, -1.0f, -1.0f), Irufemi::Vector3(-1.0f, -1.0f, -1.0f),
                Irufemi::Vector3(0.0f, 0.0f, 0.0f), Irufemi::Vector3(0.0f, 0.0f, 0.0f),
                0, 0
            });
        }
    }
    batchCollisionSettings_ = newSettings;

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

                Irufemi::Vector3 origPos, origRot, origScale;
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
        // placementType is tracked but unused since manual placement defines Irufemi::Transform
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

void EnvironmentManagerComponent::Draw() {
    // 1. 各バッチレンダラーのインスタンスリストをクリア
    for (auto& pair : batchRenderers_) {
        if (pair.second) {
            pair.second->ClearInstances();
        }
    }

    // 2. 管理下のオブジェクトから Irufemi::Transform を取得し、バッチに登録
    for (const auto& info : spawnedObjects_) {
        if (auto obj = info.obj.lock()) {
            if (auto meshRenderer = obj->GetComponent<MeshRendererComponent>()) {
                // 個別の描画をストップ（Raycast判定などは生きたまま）
                meshRenderer->SetVisible(false);

                std::string modelName = meshRenderer->GetModelName();
                if (modelName.empty()) continue;

                // 未登録のモデルならバッチレンダラーを新規作成
                if (batchRenderers_.find(modelName) == batchRenderers_.end() || !batchRenderers_[modelName]) {
                    auto batchRenderer = std::make_unique<ModelBatchRendererComponent>();
                    // ModelBatchRendererComponent 自体の初期化
                    batchRenderer->SetGameObject(gameObject_);
                    batchRenderer->LoadModel(modelName);
                    batchRenderer->Initialize();
                    batchRenderers_[modelName] = std::move(batchRenderer);
                }

                int32_t effectType = 0;
                float effectParam = 0.0f;
                bool enableMask = false;
                if (auto effectMask = obj->GetComponent<EffectMaskComponent>()) {
                    enableMask = effectMask->GetEnableEffectMask();
                    effectType = effectMask->GetCustomEffectType();
                    effectParam = effectMask->GetCachedEffectParam();
                }

                // ワールド行列を取得してバッチにインスタンスを追加
                if (auto transform = obj->GetComponent<TransformComponent>()) {
                    batchRenderers_[modelName]->AddInstanceWorld(transform->GetWorldMatrix(), effectType, effectParam, enableMask);
                }
            }
        }
    }

    // 3. すべてのバッチレンダラーを描画
    for (auto& pair : batchRenderers_) {
        if (pair.second) {
            pair.second->Draw();
        }
    }
}
