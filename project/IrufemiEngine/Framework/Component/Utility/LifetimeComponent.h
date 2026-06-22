#pragma once
#include "Framework/Component/Component.h"
#include <string>

/**
 * @enum TimeoutAction
 * @brief 寿命を迎えた際のオブジェクトの振る舞い
 */
enum class TimeoutAction {
    Destroy, ///< 完全にオブジェクトを破棄する
    Disable  ///< オブジェクトを非アクティブ（isActive_ = false）にする（プール用）
};

/**
 * @class LifetimeComponent
 * @brief 指定した時間が経過するとオブジェクトをDestroyまたはDisableするコンポーネント
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

    void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; }
    float GetLifeTime() const { return lifeTime_; }
    
    void SetTimeoutAction(TimeoutAction action) { timeoutAction_ = action; }
    TimeoutAction GetTimeoutAction() const { return timeoutAction_; }

private:
    float lifeTime_ = 1.0f;        // 寿命（秒）
    float currentLifeTime_ = 0.0f; // 経過時間
    TimeoutAction timeoutAction_ = TimeoutAction::Destroy;
};
