#include "VoxelParticleComponentEditor.h"
#include <imgui.h>
#include "Framework/Component/Effect/VoxelParticleComponent.h"
#include <string>
#include <cstring>

void VoxelParticleComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto voxelComponent = dynamic_cast<VoxelParticleComponent*>(component);
    if (!voxelComponent) return;

    if (ImGui::CollapsingHeader("VoxelParticle Component", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        char buf[256];
        strncpy_s(buf, sizeof(buf), voxelComponent->GetOverrideModelName().c_str(), _TRUNCATE);
        if (ImGui::InputText("Override Model", buf, sizeof(buf))) {
            voxelComponent->SetOverrideModelName(buf);
        }
        
        Vector3Int resolution = voxelComponent->GetResolution();
        int res[3] = {resolution.x, resolution.y, resolution.z};
        if (ImGui::InputInt3("Resolution", res)) {
            voxelComponent->SetResolution({res[0], res[1], res[2]});
        }
        
        int preAllocate = voxelComponent->GetPreAllocateCount();
        if (ImGui::InputInt("PreAllocate Count", &preAllocate)) {
            voxelComponent->SetPreAllocateCount(preAllocate);
        }

        ImGui::Separator();
        ImGui::Text("Emitter Params");

        auto& emitterParams = voxelComponent->GetEmitterParams();

        const char* particleTypes[] = { "Default", "Building", "AshDisintegration", "FineScatter", "DebrisLargeGravity", "DebrisExplosive" };
        int currentType = static_cast<int>(emitterParams.particleType);
        if (ImGui::Combo("Particle Type", &currentType, particleTypes, IM_ARRAYSIZE(particleTypes))) {
            emitterParams.particleType = static_cast<VoxelParticleSystem::ParticleType>(currentType);
        }

        ImGui::DragFloat("LifeTime", &emitterParams.lifeTime, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Gravity", &emitterParams.gravity, 0.1f, -20.0f, 100.0f);
        ImGui::DragFloat("Dispersion", &emitterParams.dispersion, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Convergence", &emitterParams.convergence, 0.01f, 0.0f, 1.0f);
        
        if (ImGui::Button("Test Explode")) {
            voxelComponent->Explode();
        }
    }
}
