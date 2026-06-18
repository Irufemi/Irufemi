#include "TransformComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"

void TransformComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<TransformComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen);
    
    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        
        // --- Reset Button ---
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 40.0f);
        if (ImGui::Button("Reset##TR")) {
            Vector3 oldPos = comp->position_;  Vector3 newPos = {0,0,0};
            Vector3 oldRot = comp->rotation_;  Vector3 newRot = {0,0,0};
            Vector3 oldScale = comp->scale_;   Vector3 newScale = {1,1,1};
            
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                oldPos, newPos, [comp](const Vector3& v) { comp->position_ = v; }));
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                oldRot, newRot, [comp](const Vector3& v) { comp->rotation_ = v; }));
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                oldScale, newScale, [comp](const Vector3& v) { comp->scale_ = v; }));
        }
        
        // Position
        static Vector3 startPos;
        if (ImGui::DragFloat3("Position", &comp->position_.x, 0.1f)) {
            // Dragging changes the value directly but doesn't push command yet
        }
        if (ImGui::IsItemActivated()) startPos = comp->position_;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vector3 endPos = comp->position_;
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                startPos, endPos, [comp](const Vector3& v) { comp->position_ = v; }));
        }
        
        // Rotation
        static Vector3 startRot;
        Vector3 rotDegrees = {
            comp->rotation_.x * (180.0f / 3.14159265f),
            comp->rotation_.y * (180.0f / 3.14159265f),
            comp->rotation_.z * (180.0f / 3.14159265f)
        };
        if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f)) {
            comp->rotation_.x = rotDegrees.x * (3.14159265f / 180.0f);
            comp->rotation_.y = rotDegrees.y * (3.14159265f / 180.0f);
            comp->rotation_.z = rotDegrees.z * (3.14159265f / 180.0f);
        }
        if (ImGui::IsItemActivated()) startRot = comp->rotation_;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vector3 endRot = comp->rotation_;
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                startRot, endRot, [comp](const Vector3& v) { comp->rotation_ = v; }));
        }

        // Scale
        static Vector3 startScale;
        if (ImGui::DragFloat3("Scale", &comp->scale_.x, 0.1f)) {
        }
        if (ImGui::IsItemActivated()) startScale = comp->scale_;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vector3 endScale = comp->scale_;
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                startScale, endScale, [comp](const Vector3& v) { comp->scale_ = v; }));
        }

        ImGui::TreePop();
    }
}
#endif // EditorMode
