#include "Environment/EnvironmentManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Core/Math/MathFunction.h"
#include "Core/Utility/Log.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/EffectMaskComponent.h"
#include "Environment/DestructibleEnvironmentComponent.h"
#include "Player/TargetableComponent.h"

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
            if (setting.prefabPath_ == name) {
                newSettings.push_back(setting);
                found = true;
                break;
            }
        }
        if (!found) {
            newSettings.push_back({name, Irufemi::Vector3(-1.0f, -1.0f, -1.0f), Irufemi::Vector3(-1.0f, -1.0f, -1.0f),
                                   Irufemi::Vector3(0.0f, 0.0f, 0.0f), Irufemi::Vector3(0.0f, 0.0f, 0.0f), 0, 0, false,
                                   3});
        }
    }
    batchCollisionSettings_ = newSettings;

    RegisterHeader("Batch Collisions");
    for (auto& setting : batchCollisionSettings_) {
        std::string name = setting.prefabPath_;
        RegisterProperty("ColSize_" + name, &setting.collisionSize_);
        RegisterProperty("ColOffset_" + name, &setting.collisionOffset_);
        // Type_ is kept for serialization compatibility, but no longer modifies Y position.
        RegisterEnum("Type_" + name, &setting.placementType_, {"Building", "Floating"});

        RegisterProperty("IsDestructible_" + name, &setting.isDestructible_);
        RegisterProperty("SpawnCount_" + name, &setting.debrisSpawnCount_);

        RegisterProperty("PushbackMaskX_" + name, &setting.pushbackMask_.x);
        RegisterProperty("PushbackMaskY_" + name, &setting.pushbackMask_.y);
        RegisterProperty("PushbackMaskZ_" + name, &setting.pushbackMask_.z);
    }
}

void EnvironmentManagerComponent::Initialize() {}

void EnvironmentManagerComponent::Start() {
    if (!gameObject_) {
        return;
    }

    // 子オブジェクトとして配置されている環境オブジェクトを検索して追跡リストに登録
    const auto& children = gameObject_->GetChildren();
    for (const auto& child : children) {
        if (!child) {
            continue;
        }

        for (auto& setting : batchCollisionSettings_) {
            if (child->GetName().find(setting.prefabPath_) != std::string::npos) {
                // 初回のみプレハブからデフォルトのサイズを取得する
                if (setting.collisionSize_.x < 0.0f) {
                    if (auto obb = child->GetComponent<OBBColliderComponent>()) {
                        setting.collisionSize_ = obb->GetLocalSize();
                        setting.previousSize_ = setting.collisionSize_;
                        setting.collisionOffset_ = obb->GetLocalOffset();
                        setting.previousOffset_ = setting.collisionOffset_;
                    }
                }

                Irufemi::Vector3 origPos, origRot, origScale;
                if (auto transform = child->GetComponent<TransformComponent>()) {
                    origPos = transform->GetPosition();
                    origRot = transform->GetRotation();
                    origScale = transform->GetScale();
                }

                if (setting.isDestructible_) {
                    if (!child->GetComponent<TargetableComponent>()) {
                        child->AddComponent<TargetableComponent>();
                    }
                    if (auto targetable = child->GetComponent<TargetableComponent>()) {
                        targetable->SetTargetType(TargetType::Environment);
                    }

                    if (!child->GetComponent<DestructibleEnvironmentComponent>()) {
                        auto destructible = child->AddComponent<DestructibleEnvironmentComponent>();
                        destructible->SetDebrisSpawnCount(setting.debrisSpawnCount_);
                    }
                }

                spawnedObjects_.push_back({child, setting.prefabPath_, origPos, origRot, origScale});
                break;
            }
        }
    }

    // 最初のバッチ設定を適用
    for (const auto& info : spawnedObjects_) {
        if (auto obj = info.obj_.lock()) {
            if (auto obb = obj->GetComponent<OBBColliderComponent>()) {
                for (const auto& setting : batchCollisionSettings_) {
                    if (setting.prefabPath_ == info.prefabPath_) {
                        obb->SetLocalSize(setting.collisionSize_);
                        obb->SetLocalOffset(setting.collisionOffset_);
                        obb->pushbackMask_ = setting.pushbackMask_;
                        break;
                    }
                }
            }
        }
    }

    // 環境オブジェクトのモデルバッチレンダラーを事前ロード・初期化
    for (const auto& info : spawnedObjects_) {
        if (auto obj = info.obj_.lock()) {
            if (auto meshRenderer = obj->GetComponent<MeshRendererComponent>()) {
                meshRenderer->SetVisible(false); // 個別の描画を停止
                std::string modelName = meshRenderer->GetModelName();
                if (!modelName.empty()) {
                    GetOrCreateBatchRenderer(modelName);
                }
            }
        }
    }
}

void EnvironmentManagerComponent::Update() {
    bool anyChanged = false;
    for (auto& setting : batchCollisionSettings_) {
        if (setting.collisionSize_ != setting.previousSize_ || setting.collisionOffset_ != setting.previousOffset_ ||
            setting.pushbackMask_ != setting.previousPushbackMask_) {
            setting.previousSize_ = setting.collisionSize_;
            setting.previousOffset_ = setting.collisionOffset_;
            setting.previousPushbackMask_ = setting.pushbackMask_;
            anyChanged = true;
        }
        // placementType is tracked but unused since manual placement defines Irufemi::Transform
        if (setting.placementType_ != setting.previousPlacementType_) {
            setting.previousPlacementType_ = setting.placementType_;
        }
    }

    if (anyChanged) {
        for (const auto& info : spawnedObjects_) {
            if (auto obj = info.obj_.lock()) {
                if (auto obb = obj->GetComponent<OBBColliderComponent>()) {
                    for (const auto& setting : batchCollisionSettings_) {
                        if (setting.prefabPath_ == info.prefabPath_) {
                            obb->SetLocalSize(setting.collisionSize_);
                            obb->SetLocalOffset(setting.collisionOffset_);
                            obb->pushbackMask_ = setting.pushbackMask_;
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
        if (auto obj = info.obj_.lock()) {
            if (!obj->GetIsActive()) {
                continue; // 破壊された環境物は描画しない
            }
            if (auto meshRenderer = obj->GetComponent<MeshRendererComponent>()) {
                // 個別の描画をストップ（Raycast判定などは生きたまま）
                meshRenderer->SetVisible(false);

                std::string modelName = meshRenderer->GetModelName();
                auto* batchRenderer = GetOrCreateBatchRenderer(modelName);
                if (!batchRenderer) {
                    continue;
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
                    batchRenderer->AddInstanceWorld(transform->GetWorldMatrix(), effectType, effectParam, enableMask);
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

ModelBatchRendererComponent* EnvironmentManagerComponent::GetOrCreateBatchRenderer(const std::string& modelName) {
    if (modelName.empty()) {
        return nullptr;
    }

    auto it = batchRenderers_.find(modelName);
    if (it != batchRenderers_.end() && it->second) {
        return it->second.get();
    }

    auto batchRenderer = std::make_unique<ModelBatchRendererComponent>();
    batchRenderer->SetGameObject(gameObject_);
    batchRenderer->LoadModel(modelName);
    batchRenderer->Initialize();

    auto* rawPtr = batchRenderer.get();
    batchRenderers_[modelName] = std::move(batchRenderer);
    return rawPtr;
}
