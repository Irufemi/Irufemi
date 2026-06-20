#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/MathFunction.h"

/**
 * @brief HitEffectのプレハブ（ルート）にアタッチし、内包するパーティクルの同期再生などを制御するコンポーネント。
 */
class HitEffectComponent : public Component {
public:
    HitEffectComponent() = default;
    ~HitEffectComponent() override = default;

    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "HitEffectComponent"; }

    /**
     * @brief 指定座標に移動し、全エミッターのEmitBurstを同期して発火する
     * @param pos 発火する座標
     */
    void PlayAt(const Vector3& pos);

private:
    int coreFlashBurstCount_ = 1;
    int sparksBurstCount_ = 60;
    int shockwaveBurstCount_ = 1;
};
