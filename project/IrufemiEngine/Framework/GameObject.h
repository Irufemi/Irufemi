#pragma once
#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Component/Component.h"
#include "Engine/Core/System/ComponentPool.h"

class BaseScene;

/**
 * @class GameObject
 * @brief コンポーネントをアタッチできるエンティティの基底クラス
 */
class GameObject : public std::enable_shared_from_this<GameObject> {
public:
    GameObject();
    GameObject(const std::string& name);
    ~GameObject() = default;

    uint64_t GetInstanceID() const { return instanceId_; }
    const std::string& GetTag() const { return tag_; }
    void SetTag(const std::string& tag) { tag_ = tag; }

    void Initialize();
    void Start();
    void Update(bool isPlayMode = true);
    void Draw();
    void DrawOutlineMask();

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);

    /**
     * @brief 自分自身の完全なコピー(クローン)を生成する
     */
    std::shared_ptr<GameObject> Clone();

    /**
     * @brief 一意のインスタンスIDを再生成する（CloneやPrefabロード時のID重複回避用）
     * @param recursive 子オブジェクトも再帰的にIDを再生成するかどうか
     */
    void RegenerateInstanceID(bool recursive = true);

    /**
     * @brief IDが再生成された際（Clone等）に、内部で保持しているID参照を新しいIDに読み替えるためのコールバック
     * @param idMap 古いID(Key) と 新しいID(Value) の対応表
     */
    virtual void OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap);

    /**
     * @brief JSONツリー内の全 instanceId を新しいUUIDに書き換え、新旧の対応表を作成する（Clone/Prefab展開用）
     */
    static void RemapJSONInstanceIDs(nlohmann::json& j, std::unordered_map<uint64_t, uint64_t>& outIdMap);

    /**
     * @brief 新しいコンポーネントを追加する
     * @return 追加されたコンポーネントの共有ポインタ
     */
    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        std::shared_ptr<T> component;
        if constexpr (IsPooledComponent<T>::value) {
            component = ComponentPool<T>::GetInstance().Create(std::forward<Args>(args)...);
        } else {
            component = std::make_shared<T>(std::forward<Args>(args)...);
        }
        
        component->SetGameObject(this);
        
        components_.push_back(component);
        componentMap_[typeid(T)].push_back(component.get());
        
        component->OnRegisterProperties();
        component->Initialize();
        if (isActive_) {
            component->OnEnable();
        }
        return component;
    }

    /**
     * @brief 既存のコンポーネントをアタッチする (Undo用)
     */
    void AddComponent(std::shared_ptr<Component> component);

    /**
     * @brief 指定した型のコンポーネントを取得する
     */
    template<typename T>
    T* GetComponent() {
        auto it = componentMap_.find(typeid(T));
        if (it != componentMap_.end() && !it->second.empty()) {
            return static_cast<T*>(it->second.front());
        }
        return nullptr;
    }

    /**
     * @brief 自身およびすべての子孫から、指定した型のコンポーネントを1つ探して取得する
     */
    template<typename T>
    T* GetComponentInChildren() {
        if (T* comp = GetComponent<T>()) {
            return comp;
        }
        for (auto& child : children_) {
            if (T* comp = child->GetComponentInChildren<T>()) {
                return comp;
            }
        }
        return nullptr;
    }

    /**
     * @brief 自身およびすべての子孫から、指定した型のコンポーネントをすべて取得する
     */
    template<typename T>
    std::vector<T*> GetComponentsInChildren() {
        std::vector<T*> results;
        GetComponentsInChildrenRecursive<T>(results);
        return results;
    }

    /**
     * @brief コンポーネントを削除する
     */
    void RemoveComponent(Component* component);

    /**
     * @brief アタッチされているすべてのコンポーネントのリストを取得する
     */
    const std::vector<std::shared_ptr<Component>>& GetComponents() const { return components_; }

    // --- アクセッサ ---
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name);
    void SetIsActive(bool isActive);
    void SetActive(bool isActive) { SetIsActive(isActive); }
    bool GetIsActive() const { return isActive_; }

    void SetScene(BaseScene* scene);
    BaseScene* GetScene() const { return scene_; }

    // --- 親子関係 ---
    void AddChild(std::shared_ptr<GameObject> child);
    void InsertChild(std::shared_ptr<GameObject> child, size_t index);
    void RemoveChild(std::shared_ptr<GameObject> child);
    std::shared_ptr<GameObject> GetParent() const { return parent_.lock(); }
    const std::vector<std::shared_ptr<GameObject>>& GetChildren() const { return children_; }
    void SetParent(std::shared_ptr<GameObject> parent);
    size_t GetChildIndex(std::shared_ptr<GameObject> child) const;

    // --- ライフサイクル ---
    /**
     * @brief オブジェクトを破棄状態にする（現在のフレームの終わりに削除される）
     */
    void Destroy() { isDestroyed_ = true; }
    bool IsDestroyed() const { return isDestroyed_; }
    bool IsStarted() const { return isStarted_; }

    // --- イベント伝達 ---
    void SendCollisionEnter(GameObject* hitObject);
    void SendCollisionStay(GameObject* hitObject);
    void SendCollisionExit(GameObject* hitObject);

    // --- 動的生成 ---
    /**
     * @brief 所属するシーンにプレハブから新しい GameObject を生成して追加する
     */
    std::shared_ptr<GameObject> Instantiate(const std::string& prefabPath, const Irufemi::Vector3& position = {0,0,0});

    // --- エディタ用フラグ ---
    void SetIsFolder(bool isFolder) { isFolder_ = isFolder; }
    bool GetIsFolder() const { return isFolder_; }
    void SetIsLocked(bool isLocked) { isLocked_ = isLocked; }
    bool GetIsLocked() const { return isLocked_; }
    void SetIsSerializable(bool isSerializable) { isSerializable_ = isSerializable; }
    bool IsSerializable() const { return isSerializable_; }

    void SetSourcePrefabPath(const std::string& path) { sourcePrefabPath_ = path; }
    const std::string& GetSourcePrefabPath() const { return sourcePrefabPath_; }

private:
    uint64_t instanceId_ = 0;
    std::string tag_ = "Untagged";
    std::string name_ = "GameObject";
    bool isActive_ = true;
    bool isStarted_ = false;
    bool isDestroyed_ = false;
    bool isFolder_ = false;
    bool isLocked_ = false;
    bool isSerializable_ = false; // デフォルトはfalse（動的生成とみなす）
    std::string sourcePrefabPath_ = "";
    BaseScene* scene_ = nullptr;

    
    std::weak_ptr<GameObject> parent_;
    std::vector<std::shared_ptr<GameObject>> children_;

    std::vector<std::shared_ptr<Component>> components_;
    std::unordered_map<std::type_index, std::vector<Component*>> componentMap_;

private:
    template<typename T>
    void GetComponentsInChildrenRecursive(std::vector<T*>& results) {
        if (T* comp = GetComponent<T>()) {
            results.push_back(comp);
        }
        for (auto& child : children_) {
            child->GetComponentsInChildrenRecursive<T>(results);
        }
    }
};
