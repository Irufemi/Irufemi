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
            ComponentUIHelpers::DrawPropertyLabel("Effect Param", "Parameter for the individual effect");
            ImGui::TableSetColumnIndex(1);
            float param = maskComp->GetCustomEffectParam();
            if (ImGui::DragFloat("##CustomEffectParam", &param, 0.01f, 0.0f, 10.0f)) {
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<float>>(
                    maskComp->GetCustomEffectParam(),
                    param,
                    [maskComp](const float& val) { maskComp->SetCustomEffectParam(val); }
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
