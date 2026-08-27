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

    RegisterHeader("Color Grading (ToneMapping & HSV)");
    RegisterProperty("Enable Color Grading", &enableColorGrading_);
    RegisterProperty("Exposure", &exposure_).SetMinMax(0.1f, 5.0f);
    RegisterProperty("Hue", &hue_).SetMinMax(-180.0f, 180.0f);
    RegisterProperty("Saturation", &saturation_).SetMinMax(-1.0f, 2.0f);
    RegisterProperty("Value", &value_).SetMinMax(-1.0f, 2.0f);
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

    // 2. Color Grading (ToneMapping & HSV) - 色調補正系
    // ※ 優先度（Priority）は PostProcessManager 側でハードコードされているため、
    // ここでの AddActiveMode の順序は実行順序に影響しません。
    if (enableColorGrading_) {
        if (!pp->HasActiveMode(PostProcessMode::ToneMapping)) {
            pp->AddActiveMode(PostProcessMode::ToneMapping);
        }
        if (!pp->HasActiveMode(PostProcessMode::HSV)) {
            pp->AddActiveMode(PostProcessMode::HSV);
        }
        auto& tm = pp->GetToneMappingParams();
        tm.exposure = exposure_;

        auto& hsv = pp->GetHSVParams();
        hsv.hue = hue_;
        hsv.saturation = saturation_;
        hsv.value = value_;
    } else {
        pp->RemoveActiveMode(PostProcessMode::ToneMapping);
        pp->RemoveActiveMode(PostProcessMode::HSV);
    }

    // 3. Vignette - 画面演出系
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
