#pragma once
#include "Framework/Component/Component.h"
#include <vector>
#include <memory>

class GameObject;
class LockonMarkerUIComponent;

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
     * @brief キューの先頭を取り出して返す（発射用）
     */
    std::shared_ptr<GameObject> PopTarget();

private:
    std::vector<std::shared_ptr<GameObject>> queuedTargets_;
    std::shared_ptr<GameObject> hoverTarget_ = nullptr;

    void UpdateHoverTarget();

    float lockonRadius2D_ = 200.0f; ///< スクリーン上の許容半径（ピクセル）
    float weight2D_ = 1.0f;         ///< 2D距離のスコア重み
    float weight3D_ = 10.0f;        ///< 3D深度のスコア重み

    LockonMarkerUIComponent* lockonMarkerUI_ = nullptr;
};
