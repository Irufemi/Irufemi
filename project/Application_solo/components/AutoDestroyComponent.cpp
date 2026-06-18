#include "AutoDestroyComponent.h"
#include "Framework/GameObject.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"

void AutoDestroyComponent::OnRegisterProperties() {
    RegisterProperty("Life Time", &lifeTime_);
}

void AutoDestroyComponent::Initialize() {
    currentLifeTime_ = 0.0f;
}

void AutoDestroyComponent::Update() {
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;

    currentLifeTime_ += deltaTime;

    // 寿命を超えたら自身を破棄
    if (currentLifeTime_ >= lifeTime_) {
        if (gameObject_) {
            gameObject_->Destroy();
        }
    }
}

nlohmann::json AutoDestroyComponent::Serialize() {
    nlohmann::json j = Component::Serialize();
    j["lifeTime"] = lifeTime_;
    return j;
}

void AutoDestroyComponent::Deserialize(const nlohmann::json& j) {
    Component::Deserialize(j);
    if (j.contains("lifeTime")) {
        lifeTime_ = j["lifeTime"].get<float>();
    }
}
