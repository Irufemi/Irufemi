#include "Framework/Component/Effect/EffectMaskComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"

EffectMaskComponent::EffectMaskComponent() {}

EffectMaskComponent::~EffectMaskComponent() {}

void EffectMaskComponent::Initialize() {
    if (gameObject_) {
        cachedRenderer_ = gameObject_->GetComponent<MeshRendererComponent>();
    }
}

void EffectMaskComponent::Update() {
    if (!cachedRenderer_ && gameObject_) {
        cachedRenderer_ = gameObject_->GetComponent<MeshRendererComponent>();
    }

    if (enableEffectMask_ && customEffectType_ > 0) {
        if (gameObject_ && gameObject_->GetScene()) {
            auto* engine = gameObject_->GetScene()->GetEngine();
            if (engine && engine->GetPostProcessManager()) {
                uint32_t id = engine->GetPostProcessManager()->RegisterCustomEffectParams(customParams_);
                cachedEffectParam_ = static_cast<float>(id) / 255.0f;
            }
        }
    } else {
        cachedEffectParam_ = 0.0f;
    }

    if (cachedRenderer_) {
        cachedRenderer_->SetEnableEffectMask(enableEffectMask_);
        cachedRenderer_->SetCustomEffectType(customEffectType_);
        cachedRenderer_->SetCustomEffectParam(cachedEffectParam_);
    }
}

void EffectMaskComponent::ApplyToRenderer() {
    // Deprecated. Handled in Update().
}

nlohmann::json EffectMaskComponent::Serialize() {
    nlohmann::json j;
    j["enableEffectMask"] = enableEffectMask_;
    j["customEffectType"] = customEffectType_;
    j["customParams"]["color1"] = {customParams_.color1.x, customParams_.color1.y, customParams_.color1.z,
                                   customParams_.color1.w};
    j["customParams"]["color2"] = {customParams_.color2.x, customParams_.color2.y, customParams_.color2.z,
                                   customParams_.color2.w};
    j["customParams"]["param1"] = customParams_.param1;
    j["customParams"]["param2"] = customParams_.param2;
    j["customParams"]["param3"] = customParams_.param3;
    j["customParams"]["param4"] = customParams_.param4;
    return j;
}

void EffectMaskComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("enableEffectMask")) {
        enableEffectMask_ = j["enableEffectMask"];
    }
    if (j.contains("customEffectType")) {
        customEffectType_ = j["customEffectType"];
    }
    // Backward compatibility
    if (j.contains("customEffectParam")) {
        customParams_.param1 = j["customEffectParam"];
    }

    if (j.contains("customParams")) {
        auto& p = j["customParams"];
        if (p.contains("color1")) {
            customParams_.color1 = {p["color1"][0], p["color1"][1], p["color1"][2], p["color1"][3]};
        }
        if (p.contains("color2")) {
            customParams_.color2 = {p["color2"][0], p["color2"][1], p["color2"][2], p["color2"][3]};
        }
        if (p.contains("param1"))
            customParams_.param1 = p["param1"];
        if (p.contains("param2"))
            customParams_.param2 = p["param2"];
        if (p.contains("param3"))
            customParams_.param3 = p["param3"];
        if (p.contains("param4"))
            customParams_.param4 = p["param4"];
    }
}

std::shared_ptr<Component> EffectMaskComponent::Clone() {
    auto clone = std::make_shared<EffectMaskComponent>();
    clone->CopyPropertiesFrom(this);
    clone->enableEffectMask_ = this->enableEffectMask_;
    clone->customEffectType_ = this->customEffectType_;
    clone->customParams_ = this->customParams_;
    clone->cachedEffectParam_ = this->cachedEffectParam_;
    return clone;
}
