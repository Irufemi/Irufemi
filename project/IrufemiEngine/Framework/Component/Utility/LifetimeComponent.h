#pragma once
#include "Framework/Component/Component.h"
#include <string>

/**
 * @class LifetimeComponent
 * @brief 一定時間経過後に自身の GameObject を破棄する汎用コンポーネント
 */
class LifetimeComponent : public Component {
public:
    LifetimeComponent() = default;
    ~LifetimeComponent() override = default;

    void OnRegisterProperties() override;
    void Initialize() override;
    void Update() override;

    std::string GetComponentName() const override { return "LifetimeComponent"; }

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

private:
    float lifeTime_ = 1.0f;        // 寿命（秒）
    float currentLifeTime_ = 0.0f; // 経過時間
};
