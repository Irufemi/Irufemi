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
        if (ComponentUIHelpers::BeginPropertyTable("RaycastTable")) {
            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Origin");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat3("##Origin", &comp->localOffset_.x, 0.1f);
            ImGui::PopItemWidth();
            ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->localOffset_);
            ComponentUIHelpers::DrawPropertyResetButton("##OriginReset", comp->localOffset_.x != 0.0f || comp->localOffset_.y != 0.0f || comp->localOffset_.z != 0.0f, [&]() {
                Irufemi::Vector3 oldO = comp->localOffset_;
                ComponentUIHelpers::PushInstantUndo(actionManager, oldO, Irufemi::Vector3{0,0,0}, &comp->localOffset_);
            });

            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Local Direction");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat3("##Local Direction", &comp->localDirection_.x, 0.1f);
            ImGui::PopItemWidth();
            ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->localDirection_);
            ComponentUIHelpers::DrawPropertyResetButton("##DirReset", comp->localDirection_.x != 0.0f || comp->localDirection_.y != 0.0f || comp->localDirection_.z != 1.0f, [&]() {
                Irufemi::Vector3 oldD = comp->localDirection_;
                ComponentUIHelpers::PushInstantUndo(actionManager, oldD, Irufemi::Vector3{0,0,1}, &comp->localDirection_);
            });

            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Max Distance");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Max Distance", &comp->maxDistance_, 0.1f, 0.0f, 10000.0f);
            ImGui::PopItemWidth();
            ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->maxDistance_);
            ComponentUIHelpers::DrawPropertyResetButton("##DistReset", comp->maxDistance_ != 1000.0f, [&]() {
                float oldD = comp->maxDistance_;
                ComponentUIHelpers::PushInstantUndo(actionManager, oldD, 1000.0f, &comp->maxDistance_);
            });

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            bool treeOpen = ImGui::TreeNodeEx("Collision Mask", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
            
            if (treeOpen) {
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("All", ImVec2(50, 0))) {
                    ComponentUIHelpers::PushInstantUndo(actionManager, comp->mask_, 0xFFFFFFFF, &comp->mask_);
                }
                ImGui::SameLine();
                if (ImGui::Button("None", ImVec2(50, 0))) {
                    ComponentUIHelpers::PushInstantUndo(actionManager, comp->mask_, 0u, &comp->mask_);
                }

                auto* cm = comp->GetGameObject() ? comp->GetGameObject()->GetScene()->GetEngine()->GetCollisionManager() : nullptr;
                const auto& layerNames = cm ? cm->GetLayerNames() : std::vector<std::string>();
                for (int i = 0; i < layerNames.size(); ++i) {
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel(layerNames[i].c_str());
                    ImGui::TableSetColumnIndex(1);
                    bool isMasked = (comp->mask_ & (1u << i)) != 0;
                    if (ImGui::Checkbox((std::string("##Mask") + std::to_string(i)).c_str(), &isMasked)) {
                        uint32_t newMask = comp->mask_;
                        if (isMasked) newMask |= (1u << i);
                        else          newMask &= ~(1u << i);
                        ComponentUIHelpers::PushInstantUndo(actionManager, comp->mask_, newMask, &comp->mask_);
                    }
                }
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TreePop();
            }
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Hit Result");
            ImGui::TableSetColumnIndex(1); ImGui::Separator();
            ImGui::TableSetColumnIndex(2); ImGui::Separator();
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            
            if (comp->hitInfo_.isHit) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Hit: %s", 
                    comp->hitInfo_.hitObject ? comp->hitInfo_.hitObject->GetName().c_str() : "Unknown");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Dist: %.2f", comp->hitInfo_.distance);
                ImGui::Text("Pt: (%.2f, %.2f, %.2f)", comp->hitInfo_.hitPoint.x, comp->hitInfo_.hitPoint.y, comp->hitInfo_.hitPoint.z);
            } else {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "No Hit");
            }
            
            ComponentUIHelpers::EndPropertyTable();
        }
    }
    ImGui::PopID();
}
#endif // EditorMode
