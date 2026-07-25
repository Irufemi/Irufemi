#include "EffectMaskComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "Framework/Component/Effect/EffectMaskComponent.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "../Core/ComponentUIHelpers.h"

void EffectMaskComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* maskComp = static_cast<EffectMaskComponent*>(component);
    if (!maskComp) return;

    bool headerOpen = ImGui::TreeNodeEx("EffectMask", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) {
            pendingRemove = true;
        }
        ImGui::EndPopup();
    }

    if (headerOpen) {
        if (ComponentUIHelpers::BeginPropertyTable("EffectMaskTable")) {
            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Enable Mask", "Protects the object from post-process effects.");
            ImGui::TableSetColumnIndex(1);

            bool enable = maskComp->GetEnableEffectMask();
            if (ImGui::Checkbox("##EnableEffectMask", &enable)) {
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<bool>>(
                    maskComp->GetEnableEffectMask(),
                    enable,
                    [maskComp](const bool& val) { maskComp->SetEnableEffectMask(val); }
                ));
            }

            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Custom Effect", "Type of individual effect to apply");
            ImGui::TableSetColumnIndex(1);
            
            const char* effectTypes[] = {
                "None", "Grayscale", "Sepia", "Vignette", "Smoothing", "GaussianFilter",
                "DepthBasedOutline", "RadialBlur", "Dissolve", "Noise", "HSV", "ToneMapping",
                "Fade", "Slide", "Bloom", "Glitch", "DualKawaseBlur", "LuminanceBasedOutline",
                "Pixelation", "Pointillism", "Posterization", "NightVision", "Kaleidoscope",
                "ChromaticAberration", "DisplacementMap", "DirectionalBlur", "Halftone",
                "DepthOfField", "LightShafts"
            };
            
            int type = maskComp->GetCustomEffectType();
            if (ImGui::Combo("##CustomEffectType", &type, effectTypes, IM_ARRAYSIZE(effectTypes))) {
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<int32_t>>(
                    maskComp->GetCustomEffectType(),
                    type,
                    [maskComp](const int32_t& val) { maskComp->SetCustomEffectType(val); }
                ));
            }

            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Effect Params", "Detailed parameters");
            ImGui::TableSetColumnIndex(1);
            
            auto params = maskComp->GetCustomParams();
            bool changed = false;

            if (type == 8) { // Dissolve
                if (ImGui::ColorEdit4("Edge Color", &params.color1.x)) changed = true;
                if (ImGui::ColorEdit4("Bg Color", &params.color2.x)) changed = true;
                if (ImGui::DragFloat("Threshold", &params.param1, 0.01f, 0.0f, 1.0f)) changed = true;
                if (ImGui::DragFloat("Edge Range", &params.param2, 0.001f, 0.0f, 1.0f)) changed = true;
                int noiseType = (int)params.param3;
                if (ImGui::Combo("Noise Type", &noiseType, "Type 0\0Type 1\0")) {
                    params.param3 = (float)noiseType;
                    changed = true;
                }
            } else if (type == 15) { // Glitch
                if (ImGui::DragFloat("Intensity", &params.param1, 0.01f, 0.0f, 10.0f)) changed = true;
            } else if (type == 13) { // Slide
                if (ImGui::ColorEdit4("Color", &params.color1.x)) changed = true;
                if (ImGui::DragFloat("Threshold", &params.param1, 0.01f, 0.0f, 1.0f)) changed = true;
            } else if (type == 12) { // Fade
                if (ImGui::ColorEdit4("Color", &params.color1.x)) changed = true;
                if (ImGui::DragFloat("Intensity", &params.param1, 0.01f, 0.0f, 1.0f)) changed = true;
            } else if (type == 3) { // Vignette
                if (ImGui::ColorEdit4("Color", &params.color1.x)) changed = true;
                if (ImGui::DragFloat("Radius", &params.param1, 0.01f, 0.0f, 2.0f)) changed = true;
                if (ImGui::DragFloat("Softness", &params.param2, 0.01f, 0.0f, 2.0f)) changed = true;
            } else if (type == 6 || type == 17) { // Outlines
                if (ImGui::ColorEdit4("Color", &params.color1.x)) changed = true;
                if (ImGui::DragFloat("Weight", &params.param1, 0.01f, 0.0f, 10.0f)) changed = true;
            } else if (type > 0) { // Fallback for other effects
                if (ImGui::DragFloat("Param 1", &params.param1, 0.01f, 0.0f, 10.0f)) changed = true;
                if (ImGui::DragFloat("Param 2", &params.param2, 0.01f, 0.0f, 10.0f)) changed = true;
                if (ImGui::ColorEdit4("Color 1", &params.color1.x)) changed = true;
            }

            if (changed) {
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<PostProcessManager::CustomEffectParams>>(
                    maskComp->GetCustomParams(),
                    params,
                    [maskComp](const PostProcessManager::CustomEffectParams& val) { maskComp->GetCustomParams() = val; }
                ));
            }

            ComponentUIHelpers::EndPropertyTable();
        }
        ImGui::TreePop();
    }

    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(
            maskComp->GetGameObject()->shared_from_this(),
            ComponentUIHelpers::GetSharedComponent(maskComp->GetGameObject(), maskComp)
        ));
    }
}
#endif // EditorMode
