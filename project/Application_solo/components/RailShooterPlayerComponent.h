#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"

/**
 * @class RailShooterPlayerComponent
 * @brief レールシューティング用のプレイヤー制御コンポーネント（ローカルオフセット加算用）
 */
class RailShooterPlayerComponent : public Component {
public:
    RailShooterPlayerComponent() = default;
    ~RailShooterPlayerComponent() override = default;

    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "RailShooterPlayerComponent"; }

private:
    float xySpeed_ = 10.0f;           ///< 上下左右に避ける（回避運動）スピード
    Irufemi::Vector3 currentOffset_ = {0,0,0}; ///< レールの中心からどのくらいずれているか（上下左右のズレ幅）

    // 画面内を動き回れる範囲（限界値）
    Irufemi::Vector3 moveLimitMin_ = {-10.0f, -10.0f, 0.0f}; ///< 移動できる限界の左下座標
    Irufemi::Vector3 moveLimitMax_ = { 10.0f,  10.0f, 0.0f}; ///< 移動できる限界の右上座標

    // --- グラビティ操作・慣性・姿勢制御パラメータ ---
    Irufemi::Vector3 currentVelocity_ = {0.0f, 0.0f, 0.0f};
    float acceleration_ = 150.0f; // 急発進の加速度
    float friction_ = 10.0f;      // 急制動の摩擦係数
    float maxSpeed_ = 15.0f;      // 最高速度

    float rollAngle_ = 0.0f;      // 現在のロール角（左右の傾き）
    float maxRollAngle_ = 1.0f;   // 最大傾き角度（約57度）
    float hoverTimer_ = 0.0f;     // 浮遊アニメーション用の経過時間
    float hoverAmplitude_ = 0.5f; // 浮遊の揺れ幅
    float hoverFrequency_ = 2.0f; // 浮遊の揺れ速度
};
