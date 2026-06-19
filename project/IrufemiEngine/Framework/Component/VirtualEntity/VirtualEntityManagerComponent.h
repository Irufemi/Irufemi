#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Utility/ObjectPool.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include <memory>
#include <vector>
#include <functional>

class GameObject;
class ModelBatchRendererComponent;

/**
 * @struct VirtualInstance
 * @brief 仮想オブジェクトのデータ
 */
struct VirtualInstance {
    int id_;
    Vector3 position_;
    Vector3 rotation_; // Euler angles
    Vector3 scale_;
    bool isPromoted_;
    bool isDestroyed_;
    std::shared_ptr<GameObject> promotedInstance_;
};

/**
 * @class VirtualEntityManagerComponent
 * @brief 大量のインスタンスをデータとして管理し、必要に応じてGameObjectとして実体化（Promote）するコンポーネント
 */
class VirtualEntityManagerComponent : public Component {
public:
    VirtualEntityManagerComponent() = default;
    ~VirtualEntityManagerComponent() override = default;

    void Initialize() override;
    void Update() override;
    std::string GetComponentName() const override { return "VirtualEntityManagerComponent"; }

    /**
     * @brief プールとファクトリの設定を行う
     * @param poolSize プールの最大サイズ
     * @param factory GameObjectを生成するファクトリ関数
     */
    void Setup(int poolSize, std::function<std::shared_ptr<GameObject>()> factory);

    /**
     * @brief 仮想インスタンスを追加する
     * @return 割り当てられたID
     */
    int AddVirtualInstance(const Vector3& pos, const Vector3& rot = {0, 0, 0}, const Vector3& scale = {1, 1, 1});

    /**
     * @brief 仮想インスタンスを論理削除する
     */
    void RemoveVirtualInstance(int id);

    /**
     * @brief 仮想インスタンスをGameObjectに昇格させる
     * @return 昇格した実体（プールが枯渇した場合はnullptr）
     */
    std::shared_ptr<GameObject> Promote(int id);

    /**
     * @brief GameObjectを仮想インスタンスに降格させ、プールに返却する
     */
    void Demote(int id);

    /**
     * @brief データ配列を取得（アプリケーション側で固有の計算をするため）
     */
    std::vector<VirtualInstance>& GetVirtualInstances() { return virtualInstances_; }

    /**
     * @brief キャッシュされている最も古い仮想インスタンスを強制的に削除し、プールを空ける
     */
    void PurgeOldestInstance();

    /**
     * @brief プールから取得した実体（仮想インスタンスに紐付いていない場合など）を直接プールに返却する
     */
    void ReleaseGameObject(std::shared_ptr<GameObject> obj);

private:
    std::vector<VirtualInstance> virtualInstances_;
    int nextId_ = 0;

    std::unique_ptr<ObjectPool<GameObject>> pool_;
    int maxPoolSize_ = 0;
    
    ModelBatchRendererComponent* batchRenderer_ = nullptr;
};
