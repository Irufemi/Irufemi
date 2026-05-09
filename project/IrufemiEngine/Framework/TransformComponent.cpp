#include "TransformComponent.h"

#ifdef EditorMode
#include <imgui.h>

void TransformComponent::OnInspectorGUI() {
    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Position", &position_.x, 0.1f);
        
        // ユーザーが扱いやすいように度数法（Degrees）に変換して表示・編集し、ラジアンに戻す
        Vector3 rotDegrees = {
            rotation_.x * (180.0f / 3.14159265f),
            rotation_.y * (180.0f / 3.14159265f),
            rotation_.z * (180.0f / 3.14159265f)
        };
        if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f)) {
            rotation_.x = rotDegrees.x * (3.14159265f / 180.0f);
            rotation_.y = rotDegrees.y * (3.14159265f / 180.0f);
            rotation_.z = rotDegrees.z * (3.14159265f / 180.0f);
        }

        ImGui::DragFloat3("Scale", &scale_.x, 0.1f);
        ImGui::TreePop();
    }
}
#endif
