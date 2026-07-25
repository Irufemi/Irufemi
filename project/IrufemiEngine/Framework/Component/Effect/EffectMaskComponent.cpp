#include "EffectMaskComponent.h"
#include "../../GameObject.h"
#include "../Renderer/MeshRendererComponent.h"
// 必要に応じてPrimitiveRenderer等もインクルード

EffectMaskComponent::EffectMaskComponent() {}

EffectMaskComponent::~EffectMaskComponent() {}

void EffectMaskComponent::Initialize() {
    if (gameObject_) {
        cachedRenderer_ = gameObject_->GetComponent<MeshRendererComponent>();
    }
    ApplyToRenderer();
}

void EffectMaskComponent::Update() {
    // 実行中にRendererが追加されたり変更された場合の対応
    if (!cachedRenderer_ && gameObject_) {
        cachedRenderer_ = gameObject_->GetComponent<MeshRendererComponent>();
        if (cachedRenderer_) {
            ApplyToRenderer();
        }
    }
}

void EffectMaskComponent::ApplyToRenderer() {
    if (cachedRenderer_) {
        cachedRenderer_->SetEnableEffectMask(enableEffectMask_);
        cachedRenderer_->SetCustomEffectType(customEffectType_);
        cachedRenderer_->SetCustomEffectParam(customEffectParam_);
    }
}

nlohmann::json EffectMaskComponent::Serialize() {
    nlohmann::json j;
    j["enableEffectMask"] = enableEffectMask_;
    j["customEffectType"] = customEffectType_;
    j["customEffectParam"] = customEffectParam_;
    return j;
}

void EffectMaskComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("enableEffectMask")) {
        enableEffectMask_ = j["enableEffectMask"];
    }
    if (j.contains("customEffectType")) {
        customEffectType_ = j["customEffectType"];
    }
    if (j.contains("customEffectParam")) {
        customEffectParam_ = j["customEffectParam"];
    }
    ApplyToRenderer();
}
