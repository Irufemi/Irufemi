#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/MathFunction.h"
#include <string>

/**
 * @brief 演出（エフェクト）の生成に特化したコンポーネント。
 *        パラメータをエディタ上で調整可能にしてデータ駆動での演出管理を行う。
 */
class EffectSpawnerComponent : public Component {
public:
    EffectSpawnerComponent() = default;
    ~EffectSpawnerComponent() override = default;

    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "EffectSpawnerComponent"; }

    /**
     * @brief 指定したワールド座標を基準にエフェクトを生成する
     * @param hitPosition 生成の基準となるワールド座標
     * @param target （オプション）生成されたエフェクトを子として追従させる対象
     */
    void PlayEffect(const Vector3& hitPosition, GameObject* target = nullptr);

private:
    std::string prefabPath_ = "resources/prefabs/hit_effect.json";
    Vector3 positionOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 baseRotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 baseScale_ = { 1.0f, 1.0f, 1.0f };
    bool attachToTarget_ = false;
};
