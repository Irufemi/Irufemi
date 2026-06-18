#include "GameObject.h"
#include "BaseScene.h"

#include "Component/Component.h"
#include "Component/ComponentFactory.h"
#include "Component/TransformComponent.h"
#include "Component/Renderer/MeshRendererComponent.h"
#include "Component/Renderer/PrimitiveRendererComponent.h"
#include "Component/Renderer/SpriteRendererComponent.h"
#include "Component/Collider/AABBColliderComponent.h"
#include "Component/Collider/SphereColliderComponent.h"
#include "Component/Collider/OBBColliderComponent.h"
#include "Component/Collider/RaycastComponent.h"
#include "Engine/IrufemiEngine.h"
#include <atomic>

static std::atomic<uint64_t> s_nextInstanceId{ 1 };

GameObject::GameObject() : instanceId_(s_nextInstanceId++) {
}

GameObject::GameObject(const std::string& name) : instanceId_(s_nextInstanceId++), name_(name) {
}

void GameObject::Initialize() {
    for (auto& comp : components_) {
        comp->Initialize();
    }
    for (auto& child : children_) {
        child->Initialize();
    }
}

void GameObject::SetName(const std::string& name) {
    if (name_ == name) return;
    std::string oldName = name_;
    name_ = name;
    if (scene_) {
        scene_->OnGameObjectNameChanged(shared_from_this(), oldName, name);
    }
}

void GameObject::SetScene(BaseScene* scene) {
    scene_ = scene;
    for (auto& child : children_) {
        if (child) {
            child->SetScene(scene);
        }
    }
}

void GameObject::Update(bool isPlayMode) {
    if (!isActive_) return;

    bool isPaused = false;
    if (scene_) {
        if (auto engine = scene_->GetEngine()) {
            isPaused = (engine->GetTimeScale() == 0.0f);
        }
    }

    for (auto& comp : components_) {
        // PlayModeでない場合は、エディタで更新可能なコンポーネントのみ更新する
        if (!isPlayMode && !comp->CanUpdateInEditMode()) {
            continue;
        }

        // ポーズ中（TimeScale == 0.0f）かつ、ポーズ中も動作する設定になっていない場合はスキップ
        if (isPlayMode && isPaused && !comp->CanUpdateWhenPaused()) {
            continue;
        }

        comp->Update();
    }
    for (auto& child : children_) {
        child->Update(isPlayMode);
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

void GameObject::DrawOutlineMask() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->DrawOutlineMask();
    }
    for (auto& child : children_) {
        child->DrawOutlineMask();
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

void GameObject::InsertChild(std::shared_ptr<GameObject> child, size_t index) {
    if (!child) return;

    if (auto currentParent = child->GetParent()) {
        currentParent->RemoveChild(child);
    }

    child->parent_ = shared_from_this();
    if (index >= children_.size()) {
        children_.push_back(child);
    } else {
        children_.insert(children_.begin() + index, child);
    }
}

void GameObject::RemoveChild(std::shared_ptr<GameObject> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_.reset();
        children_.erase(it);
    }
}

size_t GameObject::GetChildIndex(std::shared_ptr<GameObject> child) const {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        return std::distance(children_.begin(), it);
    }
    return (size_t)-1;
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

void GameObject::AddComponent(std::shared_ptr<Component> component) {
    if (!component) return;
    component->SetGameObject(this);
    components_.push_back(component);
    componentMap_[typeid(*component)].push_back(component.get());
    component->OnRegisterProperties();
    component->Initialize();
}

void GameObject::RemoveComponent(Component* component) {
    if (!component) return;

    // TransformComponentは基本として削除不可とする
    if (component->GetComponentName() == "TransformComponent") return;

    // componentMap_からの削除
    auto typeIt = componentMap_.find(typeid(*component));
    if (typeIt != componentMap_.end()) {
        auto& vec = typeIt->second;
        vec.erase(std::remove(vec.begin(), vec.end(), component), vec.end());
    }

    // components_からの削除
    components_.erase(std::remove_if(components_.begin(), components_.end(),
        [component](const std::shared_ptr<Component>& ptr) {
            return ptr.get() == component;
        }), components_.end());
}



nlohmann::json GameObject::Serialize() const {
    nlohmann::json j;
    
    // デフォルト値と異なる場合のみ出力
    if (!name_.empty()) j["name"] = name_;
    if (!tag_.empty()) j["tag"] = tag_;
    if (!isActive_) j["isActive"] = isActive_; // default is true
    if (isFolder_) j["isFolder"] = isFolder_;   // default is false
    if (isLocked_) j["isLocked"] = isLocked_;   // default is false
    
    if (!components_.empty()) {
        nlohmann::json comps = nlohmann::json::array();
        for (const auto& comp : components_) {
            nlohmann::json cj;
            cj["type"] = comp->GetComponentName();
            nlohmann::json cdata = comp->Serialize();
            // コンポーネントのデータが空でなければ出力
            if (!cdata.empty() && !cdata.is_null()) {
                cj["data"] = cdata;
            }
            comps.push_back(cj);
        }
        if (!comps.empty()) {
            j["components"] = comps;
        }
    }
    
    if (!children_.empty()) {
        nlohmann::json childrenJson = nlohmann::json::array();
        for (const auto& child : children_) {
            if (child && child->IsSerializable()) {
                childrenJson.push_back(child->Serialize());
            }
        }
        if (!childrenJson.empty()) {
            j["children"] = childrenJson;
        }
    }
    
    return j;
}

void GameObject::Deserialize(const nlohmann::json& j) {
    if (j.contains("name")) name_ = j["name"];
    if (j.contains("tag")) tag_ = j["tag"];
    if (j.contains("isActive")) isActive_ = j["isActive"];
    if (j.contains("isFolder")) isFolder_ = j["isFolder"];
    if (j.contains("isLocked")) isLocked_ = j["isLocked"];
    
    if (j.contains("components")) {
        std::vector<std::shared_ptr<Component>> loadedComps;
        for (const auto& cj : j["components"]) {
            std::string type = cj["type"];
            std::shared_ptr<Component> newComp = ComponentFactory::Create(type);
            
            if (newComp) {
                // AddComponentと同等の登録処理をInitializeの前に行う
                newComp->SetGameObject(this);
                components_.push_back(newComp);
                componentMap_[typeid(*newComp)].push_back(newComp.get());
                newComp->OnRegisterProperties();
                
                // Initialize前にパラメータを復元する
                if (cj.contains("data")) {
                    newComp->Deserialize(cj["data"]);
                }
                
                loadedComps.push_back(newComp);
            }
        }
        
        // 全てのコンポーネントがリストに登録されてから一斉にInitializeを呼ぶ
        // これにより、Initialize内でGetComponentした際に他のコンポーネントが見つかるようになる
        for (auto& comp : loadedComps) {
            comp->Initialize();
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
    
    if (scene_) {
        clone->SetName(scene_->GetUniqueObjectName(this->GetName()));
    } else {
        clone->SetName(this->GetName() + " (Clone)");
    }
    
    return clone;
}

void GameObject::SendCollisionEnter(GameObject* hitObject) {
    for (auto& comp : components_) {
        comp->OnCollisionEnter(hitObject);
    }
}

void GameObject::SendCollisionStay(GameObject* hitObject) {
    for (auto& comp : components_) {
        comp->OnCollisionStay(hitObject);
    }
}

void GameObject::SendCollisionExit(GameObject* hitObject) {
    for (auto& comp : components_) {
        comp->OnCollisionExit(hitObject);
    }
}

std::shared_ptr<GameObject> GameObject::Instantiate(const std::string& prefabPath, const Vector3& position) {
    if (scene_) {
        return scene_->InstantiatePrefab(prefabPath, position);
    }
    return nullptr;
}

