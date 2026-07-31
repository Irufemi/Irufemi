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
#include "Engine/Core/Utility/Log.h"
#include "SceneSerializer.h"
#include <iostream>
#include <atomic>
#include "Engine/Core/Math/Random/Random.h"

GameObject::GameObject() : instanceId_(Irufemi::Random::GeneratorUint64(1, ULLONG_MAX)) {
    AddComponent<TransformComponent>();
}

GameObject::GameObject(const std::string& name) : instanceId_(Irufemi::Random::GeneratorUint64(1, ULLONG_MAX)), name_(name) {
    AddComponent<TransformComponent>();
}

TransformComponent* GameObject::GetTransform() const {
    return GetComponent<TransformComponent>();
}



void GameObject::SetIsActive(bool isActive) {
    if (isActive_ == isActive) return;
    isActive_ = isActive;

    if (isActive_) {
        for (auto& comp : components_) {
            comp->OnEnable();
        }
    } else {
        for (auto& comp : components_) {
            comp->OnDisable();
        }
    }
}

void GameObject::Initialize() {
    for (auto& comp : components_) {
        comp->Initialize();
    }
    for (auto& child : children_) {
        child->Initialize();
    }
}

void GameObject::Start() {
    if (isStarted_) return;
    isStarted_ = true;

    // Use index-based loop to allow components to add components/children during Start
    for (size_t i = 0; i < components_.size(); ++i) {
        components_[i]->Start();
    }
    for (size_t i = 0; i < children_.size(); ++i) {
        children_[i]->Start();
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
    if (scene_ && !name_.empty()) {
        scene_->OnGameObjectNameChanged(shared_from_this(), "", name_);
    }
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

    for (size_t i = 0; i < components_.size(); ++i) {
        auto& comp = components_[i];
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
    
    // 破棄された子オブジェクトをリストから削除 (GC)
    children_.erase(std::remove_if(children_.begin(), children_.end(),
        [](const std::shared_ptr<GameObject>& child) {
            return !child || child->IsDestroyed();
        }), children_.end());

    for (size_t i = 0; i < children_.size(); ++i) {
        children_[i]->Update(isPlayMode);
    }
}

void GameObject::Draw() {
    if (!isActive_) return;
    for (size_t i = 0; i < components_.size(); ++i) {
        components_[i]->Draw();
    }
    for (size_t i = 0; i < children_.size(); ++i) {
        children_[i]->Draw();
    }
}

void GameObject::DrawOutlineMask() {
    if (!isActive_) return;
    for (size_t i = 0; i < components_.size(); ++i) {
        components_[i]->DrawOutlineMask();
    }
    for (size_t i = 0; i < children_.size(); ++i) {
        children_[i]->DrawOutlineMask();
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

    if (auto childTransform = child->GetComponent<TransformComponent>()) {
        childTransform->MarkWorldDirty();
    }
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

    if (auto childTransform = child->GetComponent<TransformComponent>()) {
        childTransform->MarkWorldDirty();
    }
}

void GameObject::RemoveChild(std::shared_ptr<GameObject> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_.reset();
        
        if (auto childTransform = (*it)->GetComponent<TransformComponent>()) {
            childTransform->MarkWorldDirty();
        }

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
    if (isActive_) {
        component->OnEnable();
    }
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
    
    j["instanceId"] = instanceId_;

    // デフォルト値と異なる場合のみ出力
    if (!name_.empty()) j["name"] = name_;
    if (!tag_.empty()) j["tag"] = tag_;
    if (!isActive_) j["isActive"] = isActive_; // default is true
    if (isFolder_) j["isFolder"] = isFolder_;   // default is false
    if (isLocked_) j["isLocked"] = isLocked_;   // default is false
    
    if (!sourcePrefabPath_.empty()) {
        j["prefabPath"] = sourcePrefabPath_;
        // プレハブのベースデータを取得して比較し、差分（または追加分）のみ保存する
        nlohmann::json baseJ = SceneSerializer::GetPrefabJson(sourcePrefabPath_);
        nlohmann::json baseComps = baseJ.value("components", nlohmann::json::array());

        nlohmann::json comps = nlohmann::json::array();
        for (const auto& comp : components_) {
            std::string cName = comp->GetComponentName();
            
            nlohmann::json cdata;
            try {
                cdata = comp->Serialize();
            } catch (const std::exception& e) {
                Log::OutPutLog(std::cerr, "[GameObject] Exception during Serialize of component '" + cName + "': " + std::string(e.what()) + "\n");
                continue;
            } catch (...) {
                Log::OutPutLog(std::cerr, "[GameObject] Unknown Exception during Serialize of component '" + cName + "'\n");
                continue;
            }

            if (!cdata.is_object() || cdata.empty()) continue;

            bool isOverridden = true; // プレハブに存在しない、または差分がある場合はtrue
            
            // プレハブ内の同一コンポーネントを検索
            for (const auto& baseCompJ : baseComps) {
                if (baseCompJ.value("type", "") == cName) {
                    if (baseCompJ.contains("data") && baseCompJ["data"] == cdata) {
                        isOverridden = false; // プレハブと全く同じデータ
                    }
                    break;
                }
            }

            if (isOverridden) {
                nlohmann::json cj;
                cj["type"] = cName;
                cj["data"] = cdata;
                comps.push_back(cj);
            }
        }
        if (!comps.empty()) {
            j["components"] = comps;
        }
    } else {
        if (!components_.empty()) {
            nlohmann::json comps = nlohmann::json::array();
            for (const auto& comp : components_) {
                nlohmann::json cj;
                std::string cName = comp->GetComponentName();
                cj["type"] = cName;
                
                nlohmann::json cdata;
                try {
                    cdata = comp->Serialize();
                } catch (const std::exception& e) {
                    Log::OutPutLog(std::cerr, "[GameObject] Exception during Serialize of component '" + cName + "': " + std::string(e.what()) + "\n");
                    std::cerr.flush();
                } catch (...) {
                    Log::OutPutLog(std::cerr, "[GameObject] Unknown Exception during Serialize of component '" + cName + "'\n");
                    std::cerr.flush();
                }
                
                // コンポーネントのデータが空でなければ出力
                if (cdata.is_object() && !cdata.empty()) {
                    cj["data"] = cdata;
                }
                comps.push_back(cj);
            }
            if (!comps.empty()) {
                j["components"] = comps;
            }
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
    // シリアライズから復元された＝シーンに保存されている静的オブジェクトである
    SetIsSerializable(true);

    nlohmann::json baseJ = j;
    if (j.contains("prefabPath")) {
        sourcePrefabPath_ = j["prefabPath"];
        // プレハブのベースデータを取得
        baseJ = SceneSerializer::GetPrefabJson(sourcePrefabPath_);
    }

    // まずベース(またはローカル)データから基本情報を復元
    if (baseJ.contains("name")) name_ = baseJ["name"];
    if (baseJ.contains("instanceId")) instanceId_ = baseJ["instanceId"];
    if (baseJ.contains("tag")) tag_ = baseJ["tag"];
    if (baseJ.contains("isActive")) isActive_ = baseJ["isActive"];
    if (baseJ.contains("isFolder")) isFolder_ = baseJ["isFolder"];
    if (baseJ.contains("isLocked")) isLocked_ = baseJ["isLocked"];

    // ローカル上書き情報がある場合はそれで上書き
    if (j.contains("name")) name_ = j["name"];
    if (j.contains("instanceId")) instanceId_ = j["instanceId"];
    if (j.contains("tag")) tag_ = j["tag"];
    if (j.contains("isActive")) isActive_ = j["isActive"];
    if (j.contains("isFolder")) isFolder_ = j["isFolder"];
    if (j.contains("isLocked")) isLocked_ = j["isLocked"];

    if (baseJ.contains("components")) {
        std::vector<std::shared_ptr<Component>> loadedComps;
        for (const auto& cj : baseJ["components"]) {
            std::string type = cj["type"];
            std::shared_ptr<Component> newComp;
            bool isExisting = false;

            if (type == "TransformComponent") {
                // コンストラクタで既にアタッチされているTransformを再利用する
                if (auto existingTransform = GetComponent<TransformComponent>()) {
                    for (auto& comp : components_) {
                        if (comp.get() == existingTransform) {
                            newComp = comp;
                            isExisting = true;
                            break;
                        }
                    }
                }
            }
            if (!newComp) {
                newComp = ComponentFactory::Create(type);
            }
            
            if (newComp) {
                if (!isExisting) {
                    // AddComponentと同等の登録処理をInitializeの前に行う
                    newComp->SetGameObject(this);
                    components_.push_back(newComp);
                    componentMap_[typeid(*newComp)].push_back(newComp.get());
                    newComp->OnRegisterProperties();
                }
                
                // ベースデータのプロパティを復元
                if (cj.contains("data")) {
                    newComp->Deserialize(cj["data"]);
                }

                // ローカルの上書き情報があれば反映
                if (j.contains("components")) {
                    for (const auto& localCj : j["components"]) {
                        if (localCj.contains("type") && localCj["type"] == type) {
                            if (localCj.contains("data")) {
                                newComp->Deserialize(localCj["data"]);
                            }
                            break;
                        }
                    }
                }
                
                loadedComps.push_back(newComp);
            }
        }
        
        // プレハブには存在しないが、ローカルデータで追加された新規コンポーネントを復元
        if (j.contains("components")) {
            for (const auto& localCj : j["components"]) {
                if (!localCj.contains("type")) continue;
                std::string localType = localCj["type"];
                
                // ベースデータに既に存在するかチェック
                bool existsInBase = false;
                if (baseJ.contains("components")) {
                    for (const auto& cj : baseJ["components"]) {
                        if (cj.contains("type") && cj["type"] == localType) {
                            existsInBase = true;
                            break;
                        }
                    }
                }
                
                // ベースデータに存在しない場合は新規追加
                if (!existsInBase) {
                    std::shared_ptr<Component> newComp;
                    bool isExisting = false;
                    
                    if (localType == "TransformComponent") {
                        if (auto existingTransform = GetComponent<TransformComponent>()) {
                            for (auto& comp : components_) {
                                if (comp.get() == existingTransform) {
                                    newComp = comp;
                                    isExisting = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!newComp) {
                        newComp = ComponentFactory::Create(localType);
                    }
                    
                    if (newComp) {
                        if (!isExisting) {
                            newComp->SetGameObject(this);
                            components_.push_back(newComp);
                            componentMap_[typeid(*newComp)].push_back(newComp.get());
                            newComp->OnRegisterProperties();
                        }
                        
                        if (localCj.contains("data")) {
                            newComp->Deserialize(localCj["data"]);
                        }
                        
                        loadedComps.push_back(newComp);
                    }
                }
            }
        }

        // 全てのコンポーネントがリストに登録されてから一斉にInitializeを呼ぶ
        // これにより、Initialize内でGetComponentした際に他のコンポーネントが見つかるようになる
        for (auto& comp : loadedComps) {
            comp->Initialize();
        }
        if (isActive_) {
            for (auto& comp : loadedComps) {
                comp->OnEnable();
            }
        }
    }
    
    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& cj : j["children"]) {
            auto child = std::make_shared<GameObject>();
            if (scene_) child->SetScene(scene_);
            child->Deserialize(cj);
            AddChild(child);
        }
    }
}

std::shared_ptr<GameObject> GameObject::Clone() {
    auto clone = std::make_shared<GameObject>();
    
    nlohmann::json root = this->Serialize();
    std::unordered_map<uint64_t, uint64_t> idMap;
    GameObject::RemapJSONInstanceIDs(root, idMap);
    
    clone->Deserialize(root);
    
    // クローン元のシリアライズフラグを引き継ぐ
    clone->SetIsSerializable(this->IsSerializable());

    // --- コンポーネントにアタッチされた GameObject の参照をクローン側に差し替える ---
    if (scene_) {
        clone->SetName(scene_->GetUniqueObjectName(this->GetName()));
    } else {
        clone->SetName(this->GetName() + " (Clone)");
    }
    
    clone->OnIDRemapped(idMap);
    
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

std::shared_ptr<GameObject> GameObject::Instantiate(const std::string& prefabPath, const Irufemi::Vector3& position) {
    if (scene_) {
        return scene_->InstantiatePrefab(prefabPath, position);
    }
    return nullptr;
}

void GameObject::RegenerateInstanceID(bool recursive) {
    instanceId_ = Irufemi::Random::GeneratorUint64(1, ULLONG_MAX);
    if (recursive) {
        for (auto& child : children_) {
            child->RegenerateInstanceID(true);
        }
    }
}

void GameObject::OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) {
    for (auto& comp : components_) {
        comp->OnIDRemapped(idMap);
    }
    for (auto& child : children_) {
        child->OnIDRemapped(idMap);
    }
}

void GameObject::RemapJSONInstanceIDs(nlohmann::json& j, std::unordered_map<uint64_t, uint64_t>& outIdMap) {
    if (j.contains("instanceId")) {
        uint64_t oldId = j["instanceId"];
        uint64_t newId = Irufemi::Random::GeneratorUint64(1, ULLONG_MAX);
        j["instanceId"] = newId;
        outIdMap[oldId] = newId;
    }
    
    if (j.contains("children") && j["children"].is_array()) {
        for (auto& cj : j["children"]) {
            RemapJSONInstanceIDs(cj, outIdMap);
        }
    }
}
