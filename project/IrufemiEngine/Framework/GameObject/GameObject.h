#pragma once
#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Framework/Component/Component.h"
#include "Core/System/ComponentPool.h"

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

    /**
     * @brief InstanceID を取得する。
     * @return 取得された InstanceID
     */
    uint64_t GetInstanceID() const { return instanceId_; }
    /**
     * @brief Tag を取得する。
     * @return 取得された Tag
     */
    const std::string& GetTag() const { return tag_; }
    /**
     * @brief Tag を設定する。
     * @param[in] tag 設定する Tag の値
     */
    void SetTag(const std::string& tag) { tag_ = tag; }

    /**
     * @brief ゲームオブジェクトの初期化処理を行う。アタッチされたコンポーネント群のInitializeも呼び出される。
     */
    void Initialize();
    /**
     * @brief ゲームオブジェクトの開始処理。最初のUpdateが呼ばれる直前に1度だけ実行される。
     */
    void Start();
    /**
     * @brief ゲームオブジェクトの毎フレームの更新処理。すべてのアクティブなコンポーネントを更新する。
     */
    void Update(bool isPlayMode = true);
    /**
     * @brief Draw を実行する。
     */
    void Draw();
    /**
     * @brief DrawOutlineMask を実行する。
     */
    void DrawOutlineMask();

    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() const;
    /**
     * @brief Deserialize を実行する。
     */
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
    /**
     * @brief 指定した型の新しいコンポーネントをアタッチして返す。
     * @return アタッチされたコンポーネントのポインタ
     */
    std::shared_ptr<T> AddComponent(Args&&... args) {
        std::shared_ptr<T> component;
        /**
         * @brief constexpr を実行する。
         */
        if constexpr (IsPooledComponent<T>::value) {
            component = ComponentPool<T>::GetInstance().Create(std::forward<Args>(args)...);
        } else {
            component = std::make_shared<T>(std::forward<Args>(args)...);
        }
        
        component->SetGameObject(this);
        
        components_.push_back(component);
        componentMap_[typeid(T)].push_back(component.get());
        
        if constexpr (std::is_same_v<T, TransformComponent>) {
            transformCache_ = reinterpret_cast<TransformComponent*>(component.get());
        }
        
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
    /**
     * @brief 指定した型のコンポーネントを取得する。
     * @return 見つかった場合はそのポインタ、無ければnullptr
     */
    T* GetComponent() const {
        auto it = componentMap_.find(typeid(T));
        if (it != componentMap_.end() && !it->second.empty()) {
            return static_cast<T*>(it->second.front());
        }
        return nullptr;
    }

    /**
     * @brief 自身のアタッチされている TransformComponent を取得するショートカット
     * @return TransformComponent* (GameObjectは必ずTransformを持つため、基本的にはnullptrにならない)
     */
    class TransformComponent* GetTransform() const { return transformCache_; }

    /**
     * @brief 自身およびすべての子孫から、指定した型のコンポーネントを1つ探して取得する
     */
    template<typename T>
    /**
     * @brief ComponentInChildren を取得する。
     * @return 取得された ComponentInChildren
     */
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
    /**
     * @brief ComponentsInChildren を取得する。
     * @return 取得された ComponentsInChildren
     */
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
    /**
     * @brief Name を取得する。
     * @return 取得された Name
     */
    const std::string& GetName() const { return name_; }
    /**
     * @brief Name を設定する。
     * @param[in] name 設定する Name の値
     */
    void SetName(const std::string& name);
    /**
     * @brief IsActive を設定する。
     * @param[in] isActive 設定する IsActive の値
     */
    void SetIsActive(bool isActive);
    /**
     * @brief ゲームオブジェクトの有効/無効を切り替える。無効なオブジェクトはUpdateや描画が行われない。
     * @param[in] active trueで有効、falseで無効
     */
    void SetActive(bool isActive) { SetIsActive(isActive); }
    /**
     * @brief IsActive を取得する。
     * @return 取得された IsActive
     */
    bool GetIsActive() const { return isActive_; }

    /**
     * @brief Scene を設定する。
     * @param[in] scene 設定する Scene の値
     */
    void SetScene(BaseScene* scene);
    /**
     * @brief Scene を取得する。
     * @return 取得された Scene
     */
    BaseScene* GetScene() const { return scene_; }

    // --- 親子関係 ---
    /**
     * @brief AddChild を実行する。
     */
    void AddChild(std::shared_ptr<GameObject> child);
    /**
     * @brief InsertChild を実行する。
     */
    void InsertChild(std::shared_ptr<GameObject> child, size_t index);
    /**
     * @brief RemoveChild を実行する。
     */
    void RemoveChild(std::shared_ptr<GameObject> child);
    /**
     * @brief Parent を取得する。
     * @return 取得された Parent
     */
    std::shared_ptr<GameObject> GetParent() const { return parent_.lock(); }
    /**
     * @brief Children を取得する。
     * @return 取得された Children
     */
    const std::vector<std::shared_ptr<GameObject>>& GetChildren() const { return children_; }
    /**
     * @brief Parent を設定する。
     * @param[in] parent 設定する Parent の値
     */
    void SetParent(std::shared_ptr<GameObject> parent);
    /**
     * @brief ChildIndex を取得する。
     * @return 取得された ChildIndex
     */
    size_t GetChildIndex(std::shared_ptr<GameObject> child) const;

    // --- ライフサイクル ---
    /**
     * @brief オブジェクトを破棄状態にする（現在のフレームの終わりに削除される）
     */
    void Destroy() { isDestroyed_ = true; }
    /**
     * @brief IsDestroyed かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool IsDestroyed() const { return isDestroyed_; }
    /**
     * @brief IsStarted かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool IsStarted() const { return isStarted_; }

    // --- イベント伝達 ---
    /**
     * @brief SendCollisionEnter を実行する。
     */
    void SendCollisionEnter(GameObject* hitObject);
    /**
     * @brief SendCollisionStay を実行する。
     */
    void SendCollisionStay(GameObject* hitObject);
    /**
     * @brief SendCollisionExit を実行する。
     */
    void SendCollisionExit(GameObject* hitObject);

    // --- 動的生成 ---
    /**
     * @brief 所属するシーンにプレハブから新しい GameObject を生成して追加する
     */
    std::shared_ptr<GameObject> Instantiate(const std::string& prefabPath, const Irufemi::Vector3& position = {0,0,0});

    // --- エディタ用フラグ ---
    /**
     * @brief IsFolder を設定する。
     * @param[in] isFolder 設定する IsFolder の値
     */
    void SetIsFolder(bool isFolder) { isFolder_ = isFolder; }
    /**
     * @brief IsFolder を取得する。
     * @return 取得された IsFolder
     */
    bool GetIsFolder() const { return isFolder_; }
    /**
     * @brief IsLocked を設定する。
     * @param[in] isLocked 設定する IsLocked の値
     */
    void SetIsLocked(bool isLocked) { isLocked_ = isLocked; }
    /**
     * @brief IsLocked を取得する。
     * @return 取得された IsLocked
     */
    bool GetIsLocked() const { return isLocked_; }
    /**
     * @brief IsSerializable を設定する。
     * @param[in] isSerializable 設定する IsSerializable の値
     */
    void SetIsSerializable(bool isSerializable) { isSerializable_ = isSerializable; }
    /**
     * @brief IsSerializable かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool IsSerializable() const { return isSerializable_; }

    /**
     * @brief SourcePrefabPath を設定する。
     * @param[in] path 設定する SourcePrefabPath の値
     */
    void SetSourcePrefabPath(const std::string& path) { sourcePrefabPath_ = path; }
    /**
     * @brief SourcePrefabPath を取得する。
     * @return 取得された SourcePrefabPath
     */
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
    class TransformComponent* transformCache_ = nullptr;

private:
    template<typename T>
    /**
     * @brief ComponentsInChildrenRecursive を取得する。
     */
    void GetComponentsInChildrenRecursive(std::vector<T*>& results) {
        if (T* comp = GetComponent<T>()) {
            results.push_back(comp);
        }
        for (auto& child : children_) {
            child->GetComponentsInChildrenRecursive<T>(results);
        }
    }
};
