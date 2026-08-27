#pragma once
#include "Framework/Component/Component.h"

/**
 * @class GlobalPostProcessComponent
 * @brief シーン全体のポストプロセス（Bloom, Color Grading等）を制御するPost Process Volume
 */
class GlobalPostProcessComponent : public Component {
public:
    GlobalPostProcessComponent() = default;
    ~GlobalPostProcessComponent() override = default;

    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;

    bool CanUpdateInEditMode() const override { return true; }
    std::string GetComponentName() const override { return "GlobalPostProcessComponent"; }
    std::shared_ptr<Component> Clone() override;

private:
    // Bloom
    bool enableBloom_ = true;
    float bloomThreshold_ = 0.6f;
    float bloomIntensity_ = 1.8f;
    float bloomSigma_ = 4.0f;

    // Vignette
    bool enableVignette_ = true;
    float vignetteRadius_ = 0.8f;
    float vignetteSoftness_ = 0.5f;

    // ToneMapping & HSV (Color Grading)
    bool enableColorGrading_ = true;
    float exposure_ = 1.0f;
    float hue_ = 0.0f;
    float saturation_ = -0.1f;
    float value_ = 0.0f;
};
