#include "VoxelParticleComponent.h"
#include "../../GameObject.h"
#include "../../BaseScene.h"
#include "../TransformComponent.h"
#include "../Renderer/MeshRendererComponent.h"
#include "../Renderer/ModelBatchRendererComponent.h"
#include "../../../Engine/IrufemiEngine.h"
#include "../../../Renderer/System/VoxelParticle/VoxelParticleManager.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <iostream>

VoxelParticleComponent::VoxelParticleComponent() {
}

VoxelParticleComponent::~VoxelParticleComponent() {
}

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

void VoxelParticleComponent::Explode(const Vector3& velocity, const Vector3& rotate, const Vector3& scale) {
    if (!isInitialized_) return;

    IrufemiEngine* engine = nullptr;
    if (auto* go = GetGameObject()) {
        if (auto* scene = go->GetScene()) {
            engine = scene->GetEngine();
        }
    }

    if (engine) {
        if (auto manager = engine->GetVoxelParticleManager()) {
            Vector3 worldPos = {0,0,0};
            if (auto* go = GetGameObject()) {
                if (auto transform = go->GetComponent<TransformComponent>()) {
                    worldPos = transform->worldPosition_;
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
    if (j.contains("overrideModelName")) overrideModelName_ = j["overrideModelName"];
    if (j.contains("resolution")) {
        resolution_.x = j["resolution"][0];
        resolution_.y = j["resolution"][1];
        resolution_.z = j["resolution"][2];
    }
    if (j.contains("preAllocateCount")) preAllocateCount_ = j["preAllocateCount"];

    if (j.contains("particleType")) emitterParams_.particleType = static_cast<VoxelParticleSystem::ParticleType>(j["particleType"]);
    if (j.contains("lifeTime")) emitterParams_.lifeTime = j["lifeTime"];
    if (j.contains("gravity")) emitterParams_.gravity = j["gravity"];
    if (j.contains("dispersion")) emitterParams_.dispersion = j["dispersion"];
    if (j.contains("convergence")) emitterParams_.convergence = j["convergence"];
}

void VoxelParticleComponent::OnRegisterProperties() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("VoxelParticle Component", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        char buf[256];
        strncpy_s(buf, sizeof(buf), overrideModelName_.c_str(), _TRUNCATE);
        if (ImGui::InputText("Override Model", buf, sizeof(buf))) {
            overrideModelName_ = buf;
        }
        
        int res[3] = {resolution_.x, resolution_.y, resolution_.z};
        if (ImGui::InputInt3("Resolution", res)) {
            resolution_ = {res[0], res[1], res[2]};
        }
        
        ImGui::InputInt("PreAllocate Count", &preAllocateCount_);

        ImGui::Separator();
        ImGui::Text("Emitter Params");

        const char* particleTypes[] = { "Default", "Building", "AshDisintegration", "FineScatter", "DebrisLargeGravity", "DebrisExplosive" };
        int currentType = static_cast<int>(emitterParams_.particleType);
        if (ImGui::Combo("Particle Type", &currentType, particleTypes, IM_ARRAYSIZE(particleTypes))) {
            emitterParams_.particleType = static_cast<VoxelParticleSystem::ParticleType>(currentType);
        }

        ImGui::DragFloat("LifeTime", &emitterParams_.lifeTime, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Gravity", &emitterParams_.gravity, 0.1f, -20.0f, 100.0f);
        ImGui::DragFloat("Dispersion", &emitterParams_.dispersion, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Convergence", &emitterParams_.convergence, 0.01f, 0.0f, 1.0f);
        
        if (ImGui::Button("Test Explode")) {
            Explode();
        }
    }
#endif
}
