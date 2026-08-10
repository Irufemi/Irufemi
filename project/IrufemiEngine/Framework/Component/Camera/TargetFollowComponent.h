#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <string>

class TransformComponent;

/**
 * @class TargetFollowComponent
 * @brief 指定した名前の GameObject を一定の距離と角度で追従するカメラ用コンポーネント
 */
class TargetFollowComponent : public Component {
public:
    TargetFollowComponent() = default;
    ~TargetFollowComponent() override = default;

    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;
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
    std::string GetComponentName() const override { return "TargetFollowComponent"; }


    /**
     * @brief OnIDRemapped を実行する。
     */
    void OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) override;

private:
    uint64_t targetObjectID_ = 0; ///< 追従対象の GameObject ID
    Irufemi::Vector3 offset_ = {0.0f, 2.0f, -5.0f}; ///< ターゲットからの相対距離 (右, 上, 前)
    float followDelay_ = 0.9f; ///< 追従の遅延係数（1.0 に近いほど遅れる、0.0で即座に追従）

    TransformComponent* targetTransform_ = nullptr; ///< キャッシュ用
};
