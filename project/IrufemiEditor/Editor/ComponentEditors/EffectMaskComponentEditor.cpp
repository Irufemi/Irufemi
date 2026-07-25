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
