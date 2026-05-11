#pragma once
#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Component.h"

/**
 * @class GameObject
 * @brief コンポーネントをアタッチできるエンティティの基底クラス
 * @details ECSアーキテクチャにおけるEntityとして機能し、自身はロジックを持たずComponentを管理します。
 */
class GameObject : public std::enable_shared_from_this<GameObject> {
public:
    GameObject() = default;
    GameObject(const std::string& name) : name_(name) {}
    ~GameObject() = default;

    /**
     * @brief アタッチされている全コンポーネントの初期化
     */
    void Initialize();

    /**
     * @brief アタッチされている全コンポーネントの更新
     */
    void Update();

    /**
     * @brief アタッチされている全コンポーネントの描画
     */
    void Draw();

    /**
     * @brief Inspector(EditorMode) 用のGUI描画
     */
    void OnInspectorGUI();

    /**
     * @brief 自身と全コンポーネントをJSONにシリアライズ
     */
    nlohmann::json Serialize() const;

    /**
     * @brief JSONから自身と全コンポーネントを復元
     */
    void Deserialize(const nlohmann::json& j);

    /**
     * @brief 自分自身の完全なコピー(クローン)を生成する
     */
    std::shared_ptr<GameObject> Clone();

    /**
     * @brief 新しいコンポーネントを追加する
     * @tparam T 追加するコンポーネントの型
     * @tparam Args コンストラクタ引数の型
     * @param args コンストラクタ引数
     * @return T* 生成されたコンポーネントのポインタ
     */
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->SetGameObject(this);
        T* rawPtr = component.get();
        
        components_.push_back(std::move(component));
        componentMap_[typeid(T)].push_back(rawPtr);
        
        rawPtr->Initialize();
        return rawPtr;
    }

    /**
     * @brief 指定した型のコンポーネントを取得する
     * @tparam T 取得したいコンポーネントの型
     * @return T* 見つかったコンポーネントのポインタ（無ければnullptr）
     */
    template<typename T>
    T* GetComponent() {
        auto it = componentMap_.find(typeid(T));
        if (it != componentMap_.end() && !it->second.empty()) {
            return static_cast<T*>(it->second.front());
        }
        return nullptr;
    }

    // --- アクセッサ ---
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    void SetIsActive(bool isActive) { isActive_ = isActive; }
    bool GetIsActive() const { return isActive_; }

    // --- 親子関係 ---
    void AddChild(std::shared_ptr<GameObject> child);
    void RemoveChild(std::shared_ptr<GameObject> child);
    std::shared_ptr<GameObject> GetParent() const { return parent_.lock(); }
    const std::vector<std::shared_ptr<GameObject>>& GetChildren() const { return children_; }
    void SetParent(std::shared_ptr<GameObject> parent);

private:
    std::string name_ = "GameObject"; ///< オブジェクトの名前
    bool isActive_ = true;            ///< アクティブ状態フラグ
    
    std::weak_ptr<GameObject> parent_; ///< 親オブジェクト
    std::vector<std::shared_ptr<GameObject>> children_; ///< 子オブジェクトのリスト

    std::vector<std::unique_ptr<Component>> components_; ///< 所有するコンポーネントのリスト
    std::unordered_map<std::type_index, std::vector<Component*>> componentMap_; ///< 型検索用のマップ
};
