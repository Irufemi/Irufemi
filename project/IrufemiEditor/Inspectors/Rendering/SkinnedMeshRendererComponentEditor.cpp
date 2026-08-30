#include "Inspectors/Rendering/SkinnedMeshRendererComponentEditor.h"
#include <imgui.h>
#include <string>

void SkinnedMeshRendererComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto comp = static_cast<SkinnedMeshRendererComponent*>(component);
    if (!comp) return;

    if (auto rawObj = comp->GetRawObject()) {
        size_t meshCount = rawObj->GetMeshCount();

        if (ImGui::CollapsingHeader("Materials (Slots)", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (size_t i = 0; i < meshCount; ++i) {
                std::string slotName = "Element " + std::to_string(i);
                if (ImGui::TreeNode(slotName.c_str())) {
                    
                    bool hasOverride = comp->HasMaterialOverride(i);
                    bool wasOverride = hasOverride;

                    ImGui::Checkbox("Override Material", &hasOverride);
                    
                    if (hasOverride != wasOverride) {
                        if (hasOverride) {
                            // Enable override: clone default material
                            ObjMaterial initialMat;
                            if (const ObjMaterial* original = rawObj->GetMaterial(i)) {
                                initialMat = *original;
                            }
                            comp->SetMaterialOverride(i, initialMat);
                        } else {
                            // Disable override
                            comp->RemoveMaterialOverride(i);
                        }
                    }

                    if (hasOverride) {
                        ObjMaterial* overMat = comp->GetMaterialOverrideMutable(i);
                        if (overMat) {
                            ImGui::ColorEdit4("Color", &overMat->color.x);
                            ImGui::SliderFloat("Roughness", &overMat->roughness, 0.0f, 1.0f);
                            ImGui::SliderFloat("Metallic", &overMat->metallic, 0.0f, 1.0f);
                            bool enableLighting = overMat->enableLighting;
                            if (ImGui::Checkbox("Enable Lighting", &enableLighting)) {
                                overMat->enableLighting = enableLighting;
                            }
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }
    } else {
        ImGui::TextDisabled("Model not loaded yet.");
    }
}
