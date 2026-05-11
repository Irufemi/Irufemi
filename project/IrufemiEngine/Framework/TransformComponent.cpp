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

nlohmann::json TransformComponent::Serialize() {
    nlohmann::json j;
    j["position"] = { position_.x, position_.y, position_.z };
    j["rotation"] = { rotation_.x, rotation_.y, rotation_.z };
    j["scale"]    = { scale_.x, scale_.y, scale_.z };
    return j;
}

void TransformComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("position") && j["position"].is_array() && j["position"].size() == 3) {
        position_.x = j["position"][0];
        position_.y = j["position"][1];
        position_.z = j["position"][2];
    }
    if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() == 3) {
        rotation_.x = j["rotation"][0];
        rotation_.y = j["rotation"][1];
        rotation_.z = j["rotation"][2];
    }
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() == 3) {
        scale_.x = j["scale"][0];
        scale_.y = j["scale"][1];
        scale_.z = j["scale"][2];
    }
}
