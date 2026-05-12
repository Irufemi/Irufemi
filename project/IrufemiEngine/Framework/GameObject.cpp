#include "GameObject.h"

#ifdef EditorMode
#include <imgui.h>
#endif

#include "Component/TransformComponent.h"
#include "Component/Renderer/MeshRendererComponent.h"
#include "Component/Renderer/PrimitiveRendererComponent.h"
#include "Component/Renderer/SpriteRendererComponent.h"
#include "Component/Collider/AABBColliderComponent.h"
#include "Component/Collider/SphereColliderComponent.h"
#include "Component/Collider/OBBColliderComponent.h"
#include "Component/Collider/RaycastComponent.h"
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

void GameObject::RemoveComponent(Component* component) {
    if (!component) return;

    // TransformComponentは基本として削除不可とする
    if (component->GetComponentName() == "TransformComponent") return;

    // componentMap_からの削除
    for (auto& pair : componentMap_) {
        auto& vec = pair.second;
        vec.erase(std::remove(vec.begin(), vec.end(), component), vec.end());
    }

    // components_からの削除
    components_.erase(std::remove_if(components_.begin(), components_.end(),
        [component](const std::unique_ptr<Component>& ptr) {
            return ptr.get() == component;
        }), components_.end());
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
        bool hasSpriteRenderer = GetComponent<SpriteRendererComponent>() != nullptr;
        
        // どれか一つのレンダラーが存在するか
        bool hasAnyRenderer = hasMeshRenderer || hasPrimitiveRenderer || hasSpriteRenderer;

        // Transform
        if (!hasTransform) {
            if (ImGui::Selectable("TransformComponent")) AddComponent<TransformComponent>();
        } else {
            ImGui::TextDisabled("TransformComponent (Already added)");
        }

        ImGui::Separator();

        // Renderer カテゴリ（サブメニューでリスト化）
        if (ImGui::BeginMenu("Renderer")) {
            if (!hasAnyRenderer) {
                if (ImGui::Selectable("MeshRendererComponent")) AddComponent<MeshRendererComponent>();
                if (ImGui::Selectable("PrimitiveRendererComponent")) AddComponent<PrimitiveRendererComponent>();
                if (ImGui::Selectable("SpriteRendererComponent")) AddComponent<SpriteRendererComponent>();
            } else {
                if (hasMeshRenderer) ImGui::TextDisabled("MeshRendererComponent (Already added)");
                else if (hasPrimitiveRenderer) ImGui::TextDisabled("PrimitiveRendererComponent (Already added)");
                else if (hasSpriteRenderer) ImGui::TextDisabled("SpriteRendererComponent (Already added)");
                
                ImGui::Separator();
                ImGui::TextDisabled("Only one renderer is allowed.");
            }
            ImGui::EndMenu();
        }
        
        // 今後コンポーネントが増えたらここに追加
        if (ImGui::BeginMenu("Collider")) {
            if (!GetComponent<AABBColliderComponent>()) {
                if (ImGui::Selectable("AABBColliderComponent")) AddComponent<AABBColliderComponent>();
            } else {
                ImGui::TextDisabled("AABBColliderComponent (Already added)");
            }
            if (!GetComponent<SphereColliderComponent>()) {
                if (ImGui::Selectable("SphereColliderComponent")) AddComponent<SphereColliderComponent>();
            } else {
                ImGui::TextDisabled("SphereColliderComponent (Already added)");
            }
            if (!GetComponent<OBBColliderComponent>()) {
                if (ImGui::Selectable("OBBColliderComponent")) AddComponent<OBBColliderComponent>();
            } else {
                ImGui::TextDisabled("OBBColliderComponent (Already added)");
            }
            if (!GetComponent<RaycastComponent>()) {
                if (ImGui::Selectable("RaycastComponent")) AddComponent<RaycastComponent>();
            } else {
                ImGui::TextDisabled("RaycastComponent (Already added)");
            }
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        
        if (ImGui::BeginMenu("Remove Component")) {
            bool hasRemovable = false;
            for (auto& comp : components_) {
                if (!comp) continue;
                std::string compName = comp->GetComponentName();
                if (compName == "TransformComponent") continue; // Transformは削除不可
                
                hasRemovable = true;
                if (ImGui::Selectable(compName.c_str())) {
                    RemoveComponent(comp.get());
                    // 削除後即座にメニューを抜ける（イテレータ保護のため）
                    ImGui::EndMenu();
                    ImGui::EndPopup();
                    return;
                }
            }
            if (!hasRemovable) {
                ImGui::TextDisabled("No removable components.");
            }
            ImGui::EndMenu();
        }
        
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
            else if (type == "AABBColliderComponent") newComp = AddComponent<AABBColliderComponent>();
            else if (type == "SphereColliderComponent") newComp = AddComponent<SphereColliderComponent>();
            else if (type == "OBBColliderComponent") newComp = AddComponent<OBBColliderComponent>();
            else if (type == "RaycastComponent") newComp = AddComponent<RaycastComponent>();
            
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
