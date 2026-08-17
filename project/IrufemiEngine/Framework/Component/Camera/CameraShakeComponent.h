#pragma once

#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"
#include <vector>

class CameraComponent;

namespace Irufemi {
    class PerlinNoise;
}

struct ShakeEvent {
    float maxIntensity;
    float duration;
    float currentTime;
    float frequency;
    Irufemi::Vector3 axisIntensity;
    unsigned int seed;
};

/**
 * @class CameraShakeComponent
 * @brief AAA品質のカメラシェイク（多重加算、パーリンノイズ、イージング減衰）を実現するコンポーネント
 */
class CameraShakeComponent : public Component {
public:
    CameraShakeComponent();
    ~CameraShakeComponent() override;

    void Initialize() override;
    void Update() override;
    
    std::string GetComponentName() const override { return "CameraShakeComponent"; }

    /**
     * @brief カメラシェイクを再生します
     * @param intensity 揺れの強さ (振幅)
     * @param durationFrames 揺れの長さ (フレーム数。内部で秒に変換)
     * @param frequency 揺れの速さ (周波数)
     */
    void PlayShake(float intensity, int durationFrames, float frequency = 10.0f);
    
    /**
     * @brief カメラシェイクを再生します (秒指定)
     */
    void PlayShakeSeconds(float intensity, float durationSeconds, float frequency = 10.0f);

protected:
    void OnRegisterProperties() override;

private:
    std::vector<ShakeEvent> activeShakes_;
    CameraComponent* cameraComp_ = nullptr;
    std::shared_ptr<Irufemi::PerlinNoise> perlinNoise_;
    
    float globalIntensityMultiplier_ = 1.0f;
};
