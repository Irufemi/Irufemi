#include "Framework/Component/Effect/GlobalPostProcessComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/PostProcess/PostProcessManager.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Renderer/System/Core/BaseModel.h"

void GlobalPostProcessComponent::OnRegisterProperties() {
    RegisterHeader("Bloom Settings");
    RegisterProperty("Enable Bloom", &enableBloom_);
    RegisterProperty("Bloom Threshold", &bloomThreshold_).SetMinMax(0.0f, 10.0f);
    RegisterProperty("Bloom Intensity", &bloomIntensity_).SetMinMax(0.0f, 10.0f);
    RegisterProperty("Bloom Sigma", &bloomSigma_).SetMinMax(0.1f, 20.0f);

    RegisterHeader("Vignette Settings");
    RegisterProperty("Enable Vignette", &enableVignette_);
    RegisterProperty("Vignette Radius", &vignetteRadius_).SetMinMax(0.0f, 1.5f);
    RegisterProperty("Vignette Softness", &vignetteSoftness_).SetMinMax(0.0f, 1.0f);

}

void GlobalPostProcessComponent::Start() {
    auto engine = BaseModel::GetIrufemiEngine();
    if (engine) {
        if (auto pp = engine->GetPostProcessManager()) {
            pp->Reset(); // 以前のプレイセッションやエディタ状態で残っているエフェクトスタックを完全にクリア
        }
    }
    Update();
}

void GlobalPostProcessComponent::Update() {
    auto engine = BaseModel::GetIrufemiEngine();
    if (!engine) return;
    auto pp = engine->GetPostProcessManager();
    if (!pp) return;

    // 1. Bloom - 空間・ぼかし系
    if (enableBloom_) {
        if (!pp->HasActiveMode(PostProcessMode::Bloom)) {
            pp->AddActiveMode(PostProcessMode::Bloom);
        }
        auto& bloom = pp->GetBloomParams();
        bloom.threshold = bloomThreshold_;
        bloom.intensity = bloomIntensity_;
        bloom.sigma = bloomSigma_;
    } else {
        pp->RemoveActiveMode(PostProcessMode::Bloom);
    }

    // 2. Vignette - 画面演出系
    if (enableVignette_) {
        if (!pp->HasActiveMode(PostProcessMode::Vignette)) {
            pp->AddActiveMode(PostProcessMode::Vignette);
        }
        auto& vignette = pp->GetVignetteParams();
        vignette.radius = vignetteRadius_;
        vignette.softness = vignetteSoftness_;
    } else {
        pp->RemoveActiveMode(PostProcessMode::Vignette);
    }
}

std::shared_ptr<Component> GlobalPostProcessComponent::Clone() {
    auto clone = std::make_shared<GlobalPostProcessComponent>();
    clone->enableBloom_ = enableBloom_;
    clone->bloomThreshold_ = bloomThreshold_;
    clone->bloomIntensity_ = bloomIntensity_;
    clone->bloomSigma_ = bloomSigma_;
    clone->enableVignette_ = enableVignette_;
    clone->vignetteRadius_ = vignetteRadius_;
    clone->vignetteSoftness_ = vignetteSoftness_;
    clone->enableColorGrading_ = enableColorGrading_;
    clone->exposure_ = exposure_;
    clone->hue_ = hue_;
    clone->saturation_ = saturation_;
    clone->value_ = value_;
    return clone;
}
