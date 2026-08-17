#include "Framework/Component/Effect/ScreenEffectComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
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
    wasModeActiveBeforePlay_ = engine->GetPostProcessManager()->HasActiveMode(mode_);
    if (!wasModeActiveBeforePlay_) {
        engine->GetPostProcessManager()->AddActiveMode(mode_, PostProcessManager::Layer::PostUI);
    }
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
        
        if (!wasModeActiveBeforePlay_) {
            engine->GetPostProcessManager()->RemoveActiveMode(mode_);
        }
        return;
    }

    // Weightに基づいた補間処理
    // EaseOut的にしたい場合は、tを変化させる (例: t = t * (2 - t))
    float t = currentWeight_ * (2.0f - currentWeight_);

    if (mode_ == PostProcessMode::Glitch) {
        auto& params = engine->GetPostProcessManager()->GetGlitchParams();
        params.intensity = LerpFloat(baseGlitchParams_.intensity, targetGlitchParams_.intensity, t);
        params.edgeMaskStrength = targetGlitchParams_.edgeMaskStrength;
        params.probability = targetGlitchParams_.probability;
        params.blockSizeX = targetGlitchParams_.blockSizeX;
        params.blockSizeY = targetGlitchParams_.blockSizeY;
        params.offsetBase = targetGlitchParams_.offsetBase;
        params.offsetMax = targetGlitchParams_.offsetMax;
        params.rgbShiftBase = targetGlitchParams_.rgbShiftBase;
        params.rgbShiftMax = targetGlitchParams_.rgbShiftMax;
        params.scanlineFreq = targetGlitchParams_.scanlineFreq;
        params.scanlineIntensity = targetGlitchParams_.scanlineIntensity;
        
        params.color.x = LerpFloat(baseGlitchParams_.color.x, targetGlitchParams_.color.x, t);
        params.color.y = LerpFloat(baseGlitchParams_.color.y, targetGlitchParams_.color.y, t);
        params.color.z = LerpFloat(baseGlitchParams_.color.z, targetGlitchParams_.color.z, t);
        params.color.w = LerpFloat(baseGlitchParams_.color.w, targetGlitchParams_.color.w, t);
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
        j["targetGlitchParams"]["edgeMaskStrength"] = targetGlitchParams_.edgeMaskStrength;
        j["targetGlitchParams"]["probability"] = targetGlitchParams_.probability;
        j["targetGlitchParams"]["blockSizeX"] = targetGlitchParams_.blockSizeX;
        j["targetGlitchParams"]["blockSizeY"] = targetGlitchParams_.blockSizeY;
        j["targetGlitchParams"]["offsetBase"] = targetGlitchParams_.offsetBase;
        j["targetGlitchParams"]["offsetMax"] = targetGlitchParams_.offsetMax;
        j["targetGlitchParams"]["rgbShiftBase"] = targetGlitchParams_.rgbShiftBase;
        j["targetGlitchParams"]["rgbShiftMax"] = targetGlitchParams_.rgbShiftMax;
        j["targetGlitchParams"]["scanlineFreq"] = targetGlitchParams_.scanlineFreq;
        j["targetGlitchParams"]["scanlineIntensity"] = targetGlitchParams_.scanlineIntensity;
        j["targetGlitchParams"]["glitchColor"] = { targetGlitchParams_.color.x, targetGlitchParams_.color.y, targetGlitchParams_.color.z, targetGlitchParams_.color.w };
    } 
    else if (mode_ == PostProcessMode::Vignette) {
        j["targetVignetteParams"]["color"] = { targetVignetteParams_.color.x, targetVignetteParams_.color.y, targetVignetteParams_.color.z, targetVignetteParams_.color.w };
        j["targetVignetteParams"]["radius"] = targetVignetteParams_.radius;
        j["targetVignetteParams"]["softness"] = targetVignetteParams_.softness;
    }
    else if (mode_ == PostProcessMode::ChromaticAberration) {
        j["targetChromaticAberrationParams"]["intensity"] = targetChromaticAberrationParams_.intensity;
    }
    else if (mode_ == PostProcessMode::RadialBlur) {
        j["targetRadialBlurParams"]["blurWidth"] = targetRadialBlurParams_.blurWidth;
        j["targetRadialBlurParams"]["center"] = { targetRadialBlurParams_.center.x, targetRadialBlurParams_.center.y };
        j["targetRadialBlurParams"]["numSamples"] = targetRadialBlurParams_.numSamples;
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
        if (gj.contains("edgeMaskStrength")) targetGlitchParams_.edgeMaskStrength = gj["edgeMaskStrength"];
        if (gj.contains("probability")) targetGlitchParams_.probability = gj["probability"];
        if (gj.contains("blockSizeX")) targetGlitchParams_.blockSizeX = gj["blockSizeX"];
        if (gj.contains("blockSizeY")) targetGlitchParams_.blockSizeY = gj["blockSizeY"];
        if (gj.contains("offsetBase")) targetGlitchParams_.offsetBase = gj["offsetBase"];
        if (gj.contains("offsetMax")) targetGlitchParams_.offsetMax = gj["offsetMax"];
        if (gj.contains("rgbShiftBase")) targetGlitchParams_.rgbShiftBase = gj["rgbShiftBase"];
        if (gj.contains("rgbShiftMax")) targetGlitchParams_.rgbShiftMax = gj["rgbShiftMax"];
        if (gj.contains("scanlineFreq")) targetGlitchParams_.scanlineFreq = gj["scanlineFreq"];
        if (gj.contains("scanlineIntensity")) targetGlitchParams_.scanlineIntensity = gj["scanlineIntensity"];
        if (gj.contains("glitchColor")) {
            auto& c = gj["glitchColor"];
            targetGlitchParams_.color = { c[0], c[1], c[2], c[3] };
        }
    }

    if (j.contains("targetVignetteParams")) {
        auto& vj = j["targetVignetteParams"];
        if (vj.contains("color") && vj["color"].is_array() && vj["color"].size() == 4) {
            targetVignetteParams_.color.x = vj["color"][0];
            targetVignetteParams_.color.y = vj["color"][1];
            targetVignetteParams_.color.z = vj["color"][2];
            targetVignetteParams_.color.w = vj["color"][3];
        }
        if (vj.contains("radius")) targetVignetteParams_.radius = vj["radius"];
        if (vj.contains("softness")) targetVignetteParams_.softness = vj["softness"];
    }

    if (j.contains("targetChromaticAberrationParams")) {
        auto& cj = j["targetChromaticAberrationParams"];
        if (cj.contains("intensity")) targetChromaticAberrationParams_.intensity = cj["intensity"];
    }

    if (j.contains("targetRadialBlurParams")) {
        auto& rj = j["targetRadialBlurParams"];
        if (rj.contains("blurWidth")) targetRadialBlurParams_.blurWidth = rj["blurWidth"];
        if (rj.contains("center") && rj["center"].is_array() && rj["center"].size() == 2) {
            targetRadialBlurParams_.center.x = rj["center"][0];
            targetRadialBlurParams_.center.y = rj["center"][1];
        }
        if (rj.contains("numSamples")) targetRadialBlurParams_.numSamples = rj["numSamples"];
    }
}
