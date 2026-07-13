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
    Vector3 currentOffset_ = {0,0,0}; ///< レールの中心からどのくらいずれているか（上下左右のズレ幅）

    // 画面内を動き回れる範囲（限界値）
    Vector3 moveLimitMin_ = {-10.0f, -10.0f, 0.0f}; ///< 移動できる限界の左下座標
    Vector3 moveLimitMax_ = { 10.0f,  10.0f, 0.0f}; ///< 移動できる限界の右上座標
};
