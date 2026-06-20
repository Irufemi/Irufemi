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

    void SetParameters(int coreFlash, int sparks, int shockwave, bool showDebug) {
        coreFlashBurstCount_ = coreFlash;
        sparksBurstCount_ = sparks;
        shockwaveBurstCount_ = shockwave;
        showDebugArea_ = showDebug;
    }

private:
    int coreFlashBurstCount_ = 10;
    int sparksBurstCount_ = 60;
    int shockwaveBurstCount_ = 1;
    
    bool showDebugArea_ = true; // デバッグエリア表示の一括切り替えフラグ
};
