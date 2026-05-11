#include "GameObject.h"

#ifdef EditorMode
#include <imgui.h>
#include "TransformComponent.h"
#include "MeshRendererComponent.h"
#include "PrimitiveRendererComponent.h"
#include "SpriteRendererComponent.h"
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
        
        ImGui::Separator();
        
        bool hasSpriteRenderer = GetComponent<SpriteRendererComponent>() != nullptr;
        if (!hasSpriteRenderer) {
            if (ImGui::Selectable("SpriteRendererComponent")) AddComponent<SpriteRendererComponent>();
        } else {
            ImGui::TextDisabled("SpriteRendererComponent (Already added)");
        }
        
        // 今後コンポーネントが増えたらここに追加
        
        ImGui::EndPopup();
    }
#endif
}

nlohmann::json GameObject::Serialize() const {
    nlohmann::json j;
    j["name"] = name_;
    j["isActive"] = isActive_;
    
    nlohmann::json comps = nlohmann::json::array();
    for (const auto& comp : components_) {
        nlohmann::json cj;
        cj["type"] = comp->GetComponentName();
        cj["data"] = comp->Serialize();
        comps.push_back(cj);
    }
    j["components"] = comps;
    return j;
}

void GameObject::Deserialize(const nlohmann::json& j) {
    if (j.contains("name")) name_ = j["name"];
    if (j.contains("isActive")) isActive_ = j["isActive"];
    
    if (j.contains("components")) {
        for (const auto& cj : j["components"]) {
            std::string type = cj["type"];
            Component* newComp = nullptr;
            
            // FIXME: 将来的にはファクトリパターン等で動的に生成するのが望ましい
            if (type == "TransformComponent") newComp = AddComponent<TransformComponent>();
            else if (type == "MeshRendererComponent") newComp = AddComponent<MeshRendererComponent>();
            else if (type == "PrimitiveRendererComponent") newComp = AddComponent<PrimitiveRendererComponent>();
            else if (type == "SpriteRendererComponent") newComp = AddComponent<SpriteRendererComponent>();
            
            if (newComp && cj.contains("data")) {
                newComp->Deserialize(cj["data"]);
            }
        }
    }
}

std::shared_ptr<GameObject> GameObject::Clone() {
    auto clone = std::make_shared<GameObject>();
    clone->Deserialize(this->Serialize());
    // クローン時は必要に応じて "(Clone)" などを付与
    clone->SetName(this->GetName() + " (Clone)");
    return clone;
}
