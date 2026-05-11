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
    for (auto& child : children_) {
        child->Initialize();
    }
}

void GameObject::Update() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->Update();
    }
    for (auto& child : children_) {
        child->Update();
    }
}

void GameObject::Draw() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->Draw();
    }
    for (auto& child : children_) {
        child->Draw();
    }
}

void GameObject::AddChild(std::shared_ptr<GameObject> child) {
    if (!child) return;
    
    // 既に親がいる場合は外す
    if (auto currentParent = child->GetParent()) {
        currentParent->RemoveChild(child);
    }
    
    child->parent_ = shared_from_this();
    children_.push_back(child);
}

void GameObject::RemoveChild(std::shared_ptr<GameObject> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_.reset();
        children_.erase(it);
    }
}

void GameObject::SetParent(std::shared_ptr<GameObject> parent) {
    if (parent) {
        parent->AddChild(shared_from_this());
    } else {
        if (auto currentParent = parent_.lock()) {
            currentParent->RemoveChild(shared_from_this());
        }
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
    
    nlohmann::json childrenJson = nlohmann::json::array();
    for (const auto& child : children_) {
        childrenJson.push_back(child->Serialize());
    }
    j["children"] = childrenJson;
    
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
    
    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& cj : j["children"]) {
            auto child = std::make_shared<GameObject>();
            child->Deserialize(cj);
            AddChild(child);
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
