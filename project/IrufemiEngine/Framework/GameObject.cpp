#include "GameObject.h"

#ifdef EditorMode
#include <imgui.h>
#include "TransformComponent.h"
#include "MeshRendererComponent.h"
#include "PrimitiveRendererComponent.h"
#endif
void GameObject::Initialize() {
    for (auto& comp : components_) {
        comp->Initialize();
    }
}

void GameObject::Update() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->Update();
    }
}

void GameObject::Draw() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->Draw();
    }
}

void GameObject::OnInspectorGUI() {
#ifdef EditorMode
    for (auto& comp : components_) {
        if (comp) comp->OnInspectorGUI();
    }

    ImGui::Separator();
    ImGui::Spacing();
    
    // Add Component ボタン（横幅いっぱい）
    if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    // Add Component のポップアップメニュー
    if (ImGui::BeginPopup("AddComponentPopup")) {
        
        // 既存コンポーネントの確認
        bool hasTransform = GetComponent<TransformComponent>() != nullptr;
        bool hasMeshRenderer = GetComponent<MeshRendererComponent>() != nullptr;
        bool hasPrimitiveRenderer = GetComponent<PrimitiveRendererComponent>() != nullptr;

        // Transform
        if (!hasTransform) {
            if (ImGui::Selectable("TransformComponent")) AddComponent<TransformComponent>();
        } else {
            ImGui::TextDisabled("TransformComponent (Already added)");
        }

        ImGui::Separator();

        // MeshRenderer (Primitive があれば追加不可)
        if (!hasPrimitiveRenderer && !hasMeshRenderer) {
            if (ImGui::Selectable("MeshRendererComponent")) AddComponent<MeshRendererComponent>();
        } else if (hasPrimitiveRenderer) {
            ImGui::TextDisabled("MeshRendererComponent (Conflicts with Primitive)");
        } else {
            ImGui::TextDisabled("MeshRendererComponent (Already added)");
        }

        // PrimitiveRenderer (Mesh があれば追加不可)
        if (!hasMeshRenderer && !hasPrimitiveRenderer) {
            if (ImGui::Selectable("PrimitiveRendererComponent")) AddComponent<PrimitiveRendererComponent>();
        } else if (hasMeshRenderer) {
            ImGui::TextDisabled("PrimitiveRendererComponent (Conflicts with Mesh)");
        } else {
            ImGui::TextDisabled("PrimitiveRendererComponent (Already added)");
        }
        
        // 今後コンポーネントが増えたらここに追加
        
        ImGui::EndPopup();
    }
#endif
}
