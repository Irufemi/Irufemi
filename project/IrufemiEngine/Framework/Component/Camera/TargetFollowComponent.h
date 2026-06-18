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

    void OnRegisterProperties() override;
    void Initialize() override;
    void Update() override;

    std::string GetComponentName() const override { return "TargetFollowComponent"; }

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

private:
    std::string targetName_ = "Player"; ///< 追従対象の GameObject 名
    Vector3 offset_ = {0.0f, 2.0f, -5.0f}; ///< ターゲットからの相対距離 (右, 上, 前)
    float followDelay_ = 0.9f; ///< 追従の遅延係数（1.0 に近いほど遅れる、0.0で即座に追従）

    TransformComponent* targetTransform_ = nullptr; ///< キャッシュ用
};
