#pragma once
#include "Framework/Component/Component.h"
#include <vector>
#include <memory>
#include <future>
#include <unordered_map>

class GameObject;
class LockonMarkerUIComponent;
struct RaycastHit;

/**
 * @class PlayerTargetingComponent
 * @brief プレイヤーのマルチロックオン、手動マーキング、レイキャスト遮蔽判定を行うコンポーネント
 */
class PlayerTargetingComponent : public Component {
public:
    PlayerTargetingComponent() = default;
    ~PlayerTargetingComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;

    std::string GetComponentName() const override { return "PlayerTargetingComponent"; }

    /**
     * @brief 右クリック時に呼ばれる。スコアが最も高い敵をキューに追加する
     * @param maxLockOn ロックオンの最大上限数
     */
    void MarkTarget(size_t maxLockOn);

    /**
     * @brief 手動キャンセル(Rキー)などで呼ばれる。キューを全クリア
     */
    void ClearTargets();

    const std::vector<std::shared_ptr<GameObject>>& GetQueuedTargets() const { return queuedTargets_; }
    
    /**
     * @brief 現在レティクルが合っているホバー中のターゲットを取得する
     */
    std::shared_ptr<GameObject> GetHoverTarget() const { return hoverTarget_; }
    
    /**
     * @brief キューの先頭を取り出して返す（発射用）
     */
    std::shared_ptr<GameObject> PopTarget();

    void SetMaxLockonCount(size_t count) { maxLockonCount_ = count; }

private:
    size_t maxLockonCount_ = 1;
    std::vector<std::shared_ptr<GameObject>> queuedTargets_;
    std::shared_ptr<GameObject> hoverTarget_ = nullptr;
    
    /** @brief 非同期レイキャストの結果待機用と時間間引き(Amortization)用キャッシュ構造体 */
    struct TargetVisibilityCache {
        bool canSee = true;
        float lastCheckTime = -1.0f;
        std::shared_ptr<std::future<std::pair<bool, RaycastHit>>> pendingTask;
    };
    std::unordered_map<GameObject*, TargetVisibilityCache> visibilityCache_;

    void UpdateHoverTarget();

    float lockonRadius2D_ = 200.0f; ///< スクリーン上の許容半径（ピクセル）
    float weight2D_ = 1.0f;         ///< 2D距離のスコア重み
    float weight3D_ = 10.0f;        ///< 3D深度のスコア重み

    LockonMarkerUIComponent* lockonMarkerUI_ = nullptr;
};
