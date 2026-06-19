#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <memory>
#include <vector>
#include <queue>

class GameObject;
class VirtualEntityManagerComponent;

// がれきのアニメーション用並行データ（Data-Oriented Parallel Array）
struct DebrisAnimData {
    float baseIdleY_;
    float idleTimeY_;
};


/**
 * @class DebrisManagerComponent
 * @brief ガレキの生成・プール管理・検索を行うコンポーネント
 */
class DebrisManagerComponent : public Component {
public:
    DebrisManagerComponent() = default;
    ~DebrisManagerComponent() override = default;

    void Initialize() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "DebrisManagerComponent"; }

    // ガレキの実体取得と返却（Bossのシールド等、IDなしの取得用）
    std::shared_ptr<GameObject> AcquireDebris();
    void ReleaseDebris(std::shared_ptr<GameObject> debris);

    // プレイヤーからの引き寄せ処理用：指定座標から一番近い未昇格のがれきを実体化して返す
    std::shared_ptr<GameObject> ExtractNearestIdleDebris(const Vector3& pos, float radius);

    // 破壊通知
    void NotifyDestroyed(int virtualId);

private:
    int poolSize_ = 500;
    int maxVirtualInstances_ = 20000; // 仮想インスタンスの最大予約数

    // エンジンの基盤システム（Virtual Entity）
    VirtualEntityManagerComponent* virtualManager_ = nullptr;

    // がれき固有のアニメーションデータ（フラット配列によるキャッシュ最適化）
    std::vector<DebrisAnimData> animDataList_;

    // 生成順を追跡するためのキュー（最古のインスタンスをO(1)で特定するため）
    std::queue<int> activeIds_;
};
