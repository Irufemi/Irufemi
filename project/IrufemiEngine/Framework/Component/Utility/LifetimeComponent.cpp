#include "Framework/Component/Utility/LifetimeComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/GameObject/GameObject.h"
#include "Renderer/System/Core/BaseModel.h"

void LifetimeComponent::OnRegisterProperties() {
    RegisterProperty("Life Time", &lifeTime_);
    RegisterEnum("Timeout Action", reinterpret_cast<int*>(&timeoutAction_), {"0: Destroy", "1: Disable"});
}

void LifetimeComponent::Initialize() {
    currentLifeTime_ = 0.0f;
}

void LifetimeComponent::Update() {
    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    currentLifeTime_ += dt;
    if (currentLifeTime_ >= lifeTime_) {
        if (timeoutAction_ == TimeoutAction::Destroy) {
            gameObject_->Destroy();
        } else {
            gameObject_->SetIsActive(false);
        }
    }
}

nlohmann::json LifetimeComponent::Serialize() {
    nlohmann::json j = Component::Serialize();
    j["lifeTime"] = lifeTime_;
    j["timeoutAction"] = static_cast<int>(timeoutAction_);
    return j;
}

void LifetimeComponent::Deserialize(const nlohmann::json& j) {
    Component::Deserialize(j);
    if (j.contains("lifeTime")) {
        lifeTime_ = j["lifeTime"].get<float>();
    }
    if (j.contains("timeoutAction")) {
        timeoutAction_ = static_cast<TimeoutAction>(j["timeoutAction"].get<int>());
    }
}
