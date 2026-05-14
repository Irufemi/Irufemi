#pragma once
#include "Framework/Component/Component.h"
#include <string>

class TransformComponent;

/**
 * @class RotatorComponent
 * @brief アタッチされた GameObject を毎フレーム回転させるサンプルスクリプト
 */
class RotatorComponent : public Component {
public:
    RotatorComponent() = default;
    ~RotatorComponent() override = default;

    void Initialize() override;
    void Update() override;

    // Editモード中（Playしていない時）は回転させないため、デフォルト(false)のままにする
    // bool CanUpdateInEditMode() const override { return false; }

    std::string GetComponentName() const override { return "RotatorComponent"; }

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    // パラメータ
    float rotationSpeedX_ = 0.0f;
    float rotationSpeedY_ = 1.0f;
    float rotationSpeedZ_ = 0.0f;

private:
    TransformComponent* transform_ = nullptr;
};
