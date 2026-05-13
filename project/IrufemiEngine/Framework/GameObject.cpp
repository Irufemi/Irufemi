#include "GameObject.h"

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
