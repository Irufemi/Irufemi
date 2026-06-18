#pragma once
#include "Framework/Component/Component.h"
#include <nlohmann/json.hpp>

/**
 * @class AutoDestroyComponent
 * @brief 指定された寿命（Lifetime）が経過した後に、アタッチされているGameObjectを自動的に破棄するコンポーネント。
 */
class AutoDestroyComponent : public Component {
public:
    AutoDestroyComponent() = default;
    ~AutoDestroyComponent() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override {}

    std::string GetComponentName() const override { return "AutoDestroyComponent"; }
    void OnRegisterProperties() override;

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    // --- Getters & Setters ---
    void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; }
    float GetLifeTime() const { return lifeTime_; }

private:
    float lifeTime_ = 1.0f;
    float currentLifeTime_ = 0.0f;
};
