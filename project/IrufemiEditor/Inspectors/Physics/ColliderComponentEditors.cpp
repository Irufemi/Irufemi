#include "Inspectors/Physics/ColliderComponentEditors.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "UI/ComponentUIHelpers.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Commands/EditorActionManager.h"
#include "Commands/EditorCommands.h"

void AABBColliderComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<AABBColliderComponent*>(component);
    ImGui::PushID(comp);
    bool headerOpen = ImGui::CollapsingHeader("AABB Collider", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        ComponentUIHelpers::DrawColliderCommonProperties(comp, actionManager);
    }
    ImGui::PopID();
}

void OBBColliderComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<OBBColliderComponent*>(component);
    ImGui::PushID(comp);
    bool headerOpen = ImGui::CollapsingHeader("OBB Collider", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        ComponentUIHelpers::DrawColliderCommonProperties(comp, actionManager);
    }
    ImGui::PopID();
}

void SphereColliderComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<SphereColliderComponent*>(component);
    ImGui::PushID(comp);
    bool headerOpen = ImGui::CollapsingHeader("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        ComponentUIHelpers::DrawColliderCommonProperties(comp, actionManager);
    }
    ImGui::PopID();
}

#endif // EditorMode
