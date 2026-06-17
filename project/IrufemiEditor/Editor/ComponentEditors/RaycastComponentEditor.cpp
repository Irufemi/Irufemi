#include "RaycastComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/Collider/RaycastComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Manager/CollisionManager.h"

void RaycastComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<RaycastComponent*>(component);
    ImGui::PushID(comp);
    bool headerOpen = ImGui::CollapsingHeader("Raycast", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        ImGui::DragFloat3("Origin", &comp->localOffset_.x, 0.1f);
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->localOffset_);
        ImGui::DragFloat3("Local Direction", &comp->localDirection_.x, 0.1f);
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->localDirection_);
        ImGui::DragFloat("Max Distance", &comp->maxDistance_, 0.1f, 0.0f, 10000.0f);
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->maxDistance_);
        
        if (ImGui::TreeNode("Collision Mask")) {
            if (ImGui::Button("All")) {
                ComponentUIHelpers::PushInstantUndo(actionManager, comp->mask_, 0xFFFFFFFF, &comp->mask_);
            }
            ImGui::SameLine();
            if (ImGui::Button("None")) {
                ComponentUIHelpers::PushInstantUndo(actionManager, comp->mask_, 0u, &comp->mask_);
            }

            
            auto* cm = comp->GetGameObject() ? comp->GetGameObject()->GetScene()->GetEngine()->GetCollisionManager() : nullptr;
            const auto& layerNames = cm ? cm->GetLayerNames() : std::vector<std::string>();
            for (int i = 0; i < layerNames.size(); ++i) {
                bool isMasked = (comp->mask_ & (1u << i)) != 0;
                if (ImGui::Checkbox(layerNames[i].c_str(), &isMasked)) {
                    uint32_t newMask = comp->mask_;
                    if (isMasked) newMask |= (1u << i);
                    else          newMask &= ~(1u << i);
                    ComponentUIHelpers::PushInstantUndo(actionManager, comp->mask_, newMask, &comp->mask_);
                }
            }
            ImGui::TreePop();
        }
        
        ImGui::Separator();
        if (comp->hitInfo_.isHit) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Hit: %s", 
                comp->hitInfo_.hitObject ? comp->hitInfo_.hitObject->GetName().c_str() : "Unknown");
            ImGui::Text("Distance: %.2f", comp->hitInfo_.distance);
            ImGui::Text("Point: (%.2f, %.2f, %.2f)", comp->hitInfo_.hitPoint.x, comp->hitInfo_.hitPoint.y, comp->hitInfo_.hitPoint.z);
        } else {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "No Hit");
        }
    }
    ImGui::PopID();
}
#endif // EditorMode
