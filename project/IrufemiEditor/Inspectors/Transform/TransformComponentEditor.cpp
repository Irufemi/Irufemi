#include "Inspectors/Transform/TransformComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "UI/ComponentUIHelpers.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Commands/EditorActionManager.h"
#include "Commands/EditorCommands.h"

void TransformComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<TransformComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component"))
            pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(
            comp->GetGameObject()->shared_from_this(),
            ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        if (ComponentUIHelpers::BeginPropertyTable("TransformTable")) {
            // Position
            static Irufemi::Vector3 startPos;
            Irufemi::Vector3 pos = comp->GetPosition();
            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Position", &pos.x, 0.1f)) {
                comp->SetPosition(pos);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemActivated())
                startPos = comp->GetPosition();
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Irufemi::Vector3 endPos = comp->GetPosition();
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Irufemi::Vector3>>(
                    startPos, endPos, [comp](const Irufemi::Vector3& v) { comp->SetPosition(v); }));
            }
            ComponentUIHelpers::DrawPropertyResetButton(
                "##Pos", pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f, [&]() {
                    Irufemi::Vector3 oldPos = comp->GetPosition();
                    actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Irufemi::Vector3>>(
                        oldPos, Irufemi::Vector3{0, 0, 0},
                        [comp](const Irufemi::Vector3& v) { comp->SetPosition(v); }));
                });

            // Rotation
            static Irufemi::Vector3 startRot;
            Irufemi::Vector3 rot = comp->GetRotation();
            Irufemi::Vector3 rotDegrees = {rot.x * (180.0f / 3.14159265f), rot.y * (180.0f / 3.14159265f),
                                           rot.z * (180.0f / 3.14159265f)};
            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Rotation");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Rotation", &rotDegrees.x, 1.0f)) {
                rot.x = rotDegrees.x * (3.14159265f / 180.0f);
                rot.y = rotDegrees.y * (3.14159265f / 180.0f);
                rot.z = rotDegrees.z * (3.14159265f / 180.0f);
                comp->SetRotation(rot);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemActivated())
                startRot = comp->GetRotation();
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Irufemi::Vector3 endRot = comp->GetRotation();
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Irufemi::Vector3>>(
                    startRot, endRot, [comp](const Irufemi::Vector3& v) { comp->SetRotation(v); }));
            }
            ComponentUIHelpers::DrawPropertyResetButton(
                "##Rot", rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f, [&]() {
                    Irufemi::Vector3 oldRot = comp->GetRotation();
                    actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Irufemi::Vector3>>(
                        oldRot, Irufemi::Vector3{0, 0, 0},
                        [comp](const Irufemi::Vector3& v) { comp->SetRotation(v); }));
                });

            // Scale
            static Irufemi::Vector3 startScale;
            Irufemi::Vector3 scale = comp->GetScale();
            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Scale");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Scale", &scale.x, 0.1f)) {
                comp->SetScale(scale);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemActivated())
                startScale = comp->GetScale();
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Irufemi::Vector3 endScale = comp->GetScale();
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Irufemi::Vector3>>(
                    startScale, endScale, [comp](const Irufemi::Vector3& v) { comp->SetScale(v); }));
            }
            ComponentUIHelpers::DrawPropertyResetButton(
                "##ScaleBtn", scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f, [&]() {
                    Irufemi::Vector3 oldScale = comp->GetScale();
                    actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Irufemi::Vector3>>(
                        oldScale, Irufemi::Vector3{1, 1, 1}, [comp](const Irufemi::Vector3& v) { comp->SetScale(v); }));
                });

            ComponentUIHelpers::EndPropertyTable();
        }

        ImGui::TreePop();
    }
}
#endif // EditorMode
