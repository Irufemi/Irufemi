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
            Vector3 oldPos = comp->GetPosition();  Vector3 newPos = {0,0,0};
            Vector3 oldRot = comp->GetRotation();  Vector3 newRot = {0,0,0};
            Vector3 oldScale = comp->GetScale();   Vector3 newScale = {1,1,1};
            
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                oldPos, newPos, [comp](const Vector3& v) { comp->SetPosition(v); }));
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                oldRot, newRot, [comp](const Vector3& v) { comp->SetRotation(v); }));
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                oldScale, newScale, [comp](const Vector3& v) { comp->SetScale(v); }));
        }
        
        // Position
        static Vector3 startPos;
        Vector3 pos = comp->GetPosition();
        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            comp->SetPosition(pos);
        }
        if (ImGui::IsItemActivated()) startPos = comp->GetPosition();
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vector3 endPos = comp->GetPosition();
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                startPos, endPos, [comp](const Vector3& v) { comp->SetPosition(v); }));
        }
        
        // Rotation
        static Vector3 startRot;
        Vector3 rot = comp->GetRotation();
        Vector3 rotDegrees = {
            rot.x * (180.0f / 3.14159265f),
            rot.y * (180.0f / 3.14159265f),
            rot.z * (180.0f / 3.14159265f)
        };
        if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f)) {
            rot.x = rotDegrees.x * (3.14159265f / 180.0f);
            rot.y = rotDegrees.y * (3.14159265f / 180.0f);
            rot.z = rotDegrees.z * (3.14159265f / 180.0f);
            comp->SetRotation(rot);
        }
        if (ImGui::IsItemActivated()) startRot = comp->GetRotation();
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vector3 endRot = comp->GetRotation();
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                startRot, endRot, [comp](const Vector3& v) { comp->SetRotation(v); }));
        }

        // Scale
        static Vector3 startScale;
        Vector3 scale = comp->GetScale();
        if (ImGui::DragFloat3("Scale", &scale.x, 0.1f)) {
            comp->SetScale(scale);
        }
        if (ImGui::IsItemActivated()) startScale = comp->GetScale();
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Vector3 endScale = comp->GetScale();
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                startScale, endScale, [comp](const Vector3& v) { comp->SetScale(v); }));
        }

        ImGui::TreePop();
    }
}
#endif // EditorMode
