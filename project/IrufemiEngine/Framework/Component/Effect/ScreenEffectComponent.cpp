#include "ScreenEffectComponent.h"
#include "../../../Engine/IrufemiEngine.h"
#include "../../../Renderer/System/Core/BaseModel.h"
#include <algorithm>

ScreenEffectComponent::ScreenEffectComponent() {}

ScreenEffectComponent::~ScreenEffectComponent() {
    if (isPlaying_) {
        auto engine = BaseModel::GetIrufemiEngine();
        if (engine && engine->GetPostProcessManager()) {
            engine->GetPostProcessManager()->RemoveActiveMode(mode_);
        }
    }
}

void ScreenEffectComponent::Initialize() {
    // 外部（JSONやエディタ）から設定された値を維持するため、ここでハードコードの上書きは行わない
}

void ScreenEffectComponent::Play() {
    auto engine = BaseModel::GetIrufemiEngine();
    if (!engine || !engine->GetPostProcessManager()) return;

    if (!isBaseCached_) {
        // 現在の値をベース（基準）としてキャッシュする
        baseGlitchParams_ = engine->GetPostProcessManager()->GetGlitchParams();
        baseVignetteParams_ = engine->GetPostProcessManager()->GetVignetteParams();
        baseChromaticAberrationParams_ = engine->GetPostProcessManager()->GetChromaticAberrationParams();
        baseRadialBlurParams_ = engine->GetPostProcessManager()->GetRadialBlurParams();
        isBaseCached_ = true;
    }

    currentWeight_ = 1.0f;
    isPlaying_ = true;
    
    // UIより前にかけるか後に掛けるか。今回はPostUIレイヤーにする。
    engine->GetPostProcessManager()->AddActiveMode(mode_, PostProcessManager::Layer::PostUI);
}

void ScreenEffectComponent::Update() {
    if (!isPlaying_) return;

    auto engine = BaseModel::GetIrufemiEngine();
    if (!engine || !engine->GetPostProcessManager()) return;

    float dt = engine->GetGameDeltaTime();
    currentWeight_ -= dt / duration_;

    if (currentWeight_ <= 0.0f) {
        currentWeight_ = 0.0f;
        isPlaying_ = false;

        // パラメータを元の値に戻す
        if (mode_ == PostProcessMode::Glitch) {
            engine->GetPostProcessManager()->GetGlitchParams() = baseGlitchParams_;
        } else if (mode_ == PostProcessMode::Vignette) {
            engine->GetPostProcessManager()->GetVignetteParams() = baseVignetteParams_;
        } else if (mode_ == PostProcessMode::ChromaticAberration) {
            engine->GetPostProcessManager()->GetChromaticAberrationParams() = baseChromaticAberrationParams_;
        } else if (mode_ == PostProcessMode::RadialBlur) {
            engine->GetPostProcessManager()->GetRadialBlurParams() = baseRadialBlurParams_;
        }
        
        engine->GetPostProcessManager()->RemoveActiveMode(mode_);
        return;
    }

    // Weightに基づいた補間処理
    // EaseOut的にしたい場合は、tを変化させる (例: t = t * (2 - t))
    float t = currentWeight_ * (2.0f - currentWeight_);

    if (mode_ == PostProcessMode::Glitch) {
        auto& params = engine->GetPostProcessManager()->GetGlitchParams();
        params.intensity = LerpFloat(baseGlitchParams_.intensity, targetGlitchParams_.intensity, t);
    } 
    else if (mode_ == PostProcessMode::Vignette) {
        auto& params = engine->GetPostProcessManager()->GetVignetteParams();
        params.radius = LerpFloat(baseVignetteParams_.radius, targetVignetteParams_.radius, t);
        params.softness = LerpFloat(baseVignetteParams_.softness, targetVignetteParams_.softness, t);
        
        params.color.x = LerpFloat(baseVignetteParams_.color.x, targetVignetteParams_.color.x, t);
        params.color.y = LerpFloat(baseVignetteParams_.color.y, targetVignetteParams_.color.y, t);
        params.color.z = LerpFloat(baseVignetteParams_.color.z, targetVignetteParams_.color.z, t);
        params.color.w = LerpFloat(baseVignetteParams_.color.w, targetVignetteParams_.color.w, t);
    }
    else if (mode_ == PostProcessMode::ChromaticAberration) {
        auto& params = engine->GetPostProcessManager()->GetChromaticAberrationParams();
        params.intensity = LerpFloat(baseChromaticAberrationParams_.intensity, targetChromaticAberrationParams_.intensity, t);
    }
    else if (mode_ == PostProcessMode::RadialBlur) {
        auto& params = engine->GetPostProcessManager()->GetRadialBlurParams();
        params.blurWidth = LerpFloat(baseRadialBlurParams_.blurWidth, targetRadialBlurParams_.blurWidth, t);
        params.center.x = LerpFloat(baseRadialBlurParams_.center.x, targetRadialBlurParams_.center.x, t);
        params.center.y = LerpFloat(baseRadialBlurParams_.center.y, targetRadialBlurParams_.center.y, t);
        // numSamples は整数なのでターゲットの値をそのまま使用するか、フェードアウト中は一定にする。
        params.numSamples = targetRadialBlurParams_.numSamples;
    }
}

nlohmann::json ScreenEffectComponent::Serialize() {
    nlohmann::json j;
    j["mode"] = static_cast<int>(mode_);
    j["duration"] = duration_;

    if (mode_ == PostProcessMode::Glitch) {
        j["targetGlitchParams"]["intensity"] = targetGlitchParams_.intensity;
    } 
    else if (mode_ == PostProcessMode::Vignette) {
        j["targetVignetteParams"]["color"] = { targetVignetteParams_.color.x, targetVignetteParams_.color.y, targetVignetteParams_.color.z, targetVignetteParams_.color.w };
        j["targetVignetteParams"]["radius"] = targetVignetteParams_.radius;
        j["targetVignetteParams"]["softness"] = targetVignetteParams_.softness;
    }
    
    return j;
}

void ScreenEffectComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("mode")) {
        mode_ = static_cast<PostProcessMode>(j["mode"]);
    }
    if (j.contains("duration")) {
        duration_ = j["duration"];
    }
    
    if (j.contains("targetGlitchParams")) {
        auto& gj = j["targetGlitchParams"];
        if (gj.contains("intensity")) targetGlitchParams_.intensity = gj["intensity"];
    }

    if (j.contains("targetVignetteParams")) {
        auto& vj = j["targetVignetteParams"];
        if (vj.contains("color")) {
            targetVignetteParams_.color.x = vj["color"][0];
            targetVignetteParams_.color.y = vj["color"][1];
            targetVignetteParams_.color.z = vj["color"][2];
            targetVignetteParams_.color.w = vj["color"][3];
        }
        if (vj.contains("radius")) targetVignetteParams_.radius = vj["radius"];
        if (vj.contains("softness")) targetVignetteParams_.softness = vj["softness"];
    }
}
