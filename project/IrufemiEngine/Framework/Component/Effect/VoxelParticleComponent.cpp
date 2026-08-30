#include "Framework/Component/Effect/VoxelParticleComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Renderer/System/VoxelParticle/VoxelParticleManager.h"

#include <iostream>

VoxelParticleComponent::VoxelParticleComponent() {}

VoxelParticleComponent::~VoxelParticleComponent() {}

void VoxelParticleComponent::Initialize() {
    cachedModelName_ = GetTargetModelName();

    if (!cachedModelName_.empty()) {
        IrufemiEngine* engine = nullptr;
        if (auto* go = GetGameObject()) {
            if (auto* scene = go->GetScene()) {
                engine = scene->GetEngine();
            }
        }
        if (engine) {
            if (auto manager = engine->GetVoxelParticleManager()) {
                manager->ReservePool(cachedModelName_, resolution_, preAllocateCount_);
                isInitialized_ = true;
            }
        }
    }
}

void VoxelParticleComponent::Update() {
    // 描画・更新はVoxelParticleManagerが一括で行うため、ここでは特に何もしない
}

void VoxelParticleComponent::Draw() {
    // 描画・更新はVoxelParticleManagerが一括で行うため、ここでは特に何もしない
}

std::string VoxelParticleComponent::GetTargetModelName() {
    if (!overrideModelName_.empty()) {
        return overrideModelName_;
    }

    if (auto* go = GetGameObject()) {
        if (auto meshRenderer = go->GetComponent<MeshRendererComponent>()) {
            return meshRenderer->GetModelName();
        }
        if (auto batchRenderer = go->GetComponent<ModelBatchRendererComponent>()) {
            return batchRenderer->GetModelName();
        }
    }
    return "";
}

void VoxelParticleComponent::Emit() {
    // 拡張用：Emit（ポタポタ落ちるような挙動等）が必要になった場合はここに実装
}

void VoxelParticleComponent::Explode(const Irufemi::Vector3& velocity, const Irufemi::Vector3& rotate,
                                     const Irufemi::Vector3& scale) {
    if (!isInitialized_)
        return;

    IrufemiEngine* engine = nullptr;
    if (auto* go = GetGameObject()) {
        if (auto* scene = go->GetScene()) {
            engine = scene->GetEngine();
        }
    }

    if (engine) {
        if (auto manager = engine->GetVoxelParticleManager()) {
            Irufemi::Vector3 worldPos = {0, 0, 0};
            if (auto* go = GetGameObject()) {
                if (auto transform = go->GetComponent<TransformComponent>()) {
                    worldPos = transform->GetWorldPosition();
                }
            }
            manager->PlayExplosion(cachedModelName_, worldPos, velocity, rotate, scale, emitterParams_, resolution_);
        }
    }
}

nlohmann::json VoxelParticleComponent::Serialize() {
    nlohmann::json j;
    j["overrideModelName"] = overrideModelName_;
    j["resolution"] = {resolution_.x, resolution_.y, resolution_.z};
    j["preAllocateCount"] = preAllocateCount_;

    j["particleType"] = static_cast<uint32_t>(emitterParams_.particleType);
    j["lifeTime"] = emitterParams_.lifeTime;
    j["gravity"] = emitterParams_.gravity;
    j["dispersion"] = emitterParams_.dispersion;
    j["convergence"] = emitterParams_.convergence;

    return j;
}

void VoxelParticleComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("overrideModelName"))
        overrideModelName_ = j["overrideModelName"];
    if (j.contains("resolution")) {
        resolution_.x = j["resolution"][0];
        resolution_.y = j["resolution"][1];
        resolution_.z = j["resolution"][2];
    }
    if (j.contains("preAllocateCount"))
        preAllocateCount_ = j["preAllocateCount"];

    if (j.contains("particleType"))
        emitterParams_.particleType = j["particleType"];
    if (j.contains("lifeTime"))
        emitterParams_.lifeTime = j["lifeTime"];
    if (j.contains("gravity"))
        emitterParams_.gravity = j["gravity"];
    if (j.contains("dispersion"))
        emitterParams_.dispersion = j["dispersion"];
    if (j.contains("convergence"))
        emitterParams_.convergence = j["convergence"];
}
