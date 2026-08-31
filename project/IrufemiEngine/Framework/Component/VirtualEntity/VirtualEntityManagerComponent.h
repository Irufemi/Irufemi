#pragma once
#include "Framework/Component/Component.h"
#include "Core/Utility/ObjectPool.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4x4.h"
#include <memory>
#include <vector>
#include <functional>
#include <queue>
#include <unordered_map>

class GameObject;
class ModelBatchRendererComponent;

/**
 * @struct VirtualInstance
 * @brief 仮想オブジェクトのデータ
 */
struct VirtualInstance {
    int id_;
    Irufemi::Vector3 position_ = {0.0f, 0.0f, 0.0f};
    Irufemi::Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
    Irufemi::Vector3 scale_ = {1.0f, 1.0f, 1.0f};
    bool isPromoted_ = false;
    bool isDestroyed_;
    ObjectPool<GameObject>::Handle promotedHandle_;
};

/**
 * @class VirtualEntityManagerComponent
 * @brief 大量のインスタンスをデータとして管理し、必要に応じてGameObjectとして実体化（Promote）するコンポーネント
 */
class VirtualEntityManagerComponent : public Component {
public:
    VirtualEntityManagerComponent();
    ~VirtualEntityManagerComponent() override;

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override {
        return "VirtualEntityManagerComponent";
    }
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

    /**
     * @brief プールとファクトリの設定を行う
     * @param poolSize 実体化（GameObject）用プールの最大サイズ
     * @param maxVirtualInstances 仮想インスタンスの最大予約数
     * @param factory GameObjectを生成するファクトリ関数
     */
    void Setup(int poolSize, int maxVirtualInstances, std::function<std::shared_ptr<GameObject>()> factory);

    /**
     * @brief 仮想インスタンスを追加する
     * @return 割り当てられたID
     */
    int AddVirtualInstance(const Irufemi::Vector3& pos, const Irufemi::Vector3& rot = {0, 0, 0},
                           const Irufemi::Vector3& scale = {1, 1, 1});

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
     * @brief データ配列（密配列・連続メモリ）を取得
     */
    std::vector<VirtualInstance>& GetDenseInstances() {
        return dense_;
    }

    /**
     * @brief 仮想IDから密配列のインデックスを取得
     */
    int GetSparseIndex(int virtualId) const {
        if (virtualId >= 0 && virtualId < sparse_.size()) {
            return sparse_[virtualId];
        }
        return -1;
    }

    /**
     * @brief プールから取得した実体（仮想インスタンスに紐付いていない場合など）を直接プールに返却する
     */
    void ReleaseGameObject(std::shared_ptr<GameObject> obj);

    /**
     * @brief 現在アクティブな全ての仮想インスタンスの総数を取得する
     */
    static int GetTotalActiveVirtualInstances();

private:
    /**
     * @brief コンポーネントの全インスタンスを保持する静的レジストリ
     */
    static std::vector<VirtualEntityManagerComponent*> sInstances_;
    std::vector<VirtualInstance> dense_;
    std::vector<int> sparse_;
    std::queue<int> freeIds_;
    int activeInstanceCount_ = 0;
    int maxVirtualInstances_ = 0;
    int nextId_ = 0; // Backup if freeIds is empty or we don't want strict pre-alloc

    std::unique_ptr<ObjectPool<GameObject>> pool_;
    int maxPoolSize_ = 0;

    ModelBatchRendererComponent* batchRenderer_ = nullptr;

    std::unordered_map<GameObject*, ObjectPool<GameObject>::Handle> activeHandles_;
};
