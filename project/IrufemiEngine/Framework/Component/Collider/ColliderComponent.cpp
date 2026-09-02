#include "Framework/Component/Collider/ColliderComponent.h"
#include "Physics/CollisionManager.h"

ColliderComponent::~ColliderComponent() {
    if (collisionManager_) {
        collisionManager_->UnregisterCollider(this);
    }
}

void ColliderComponent::OnRegisterProperties() {
    RegisterProperty("Is Trigger", &isTrigger_);
    RegisterProperty("Is Static", &isStatic_);
    RegisterProperty("Pushback Mask X", &pushbackMask_.x);
    RegisterProperty("Pushback Mask Y", &pushbackMask_.y);
    RegisterProperty("Pushback Mask Z", &pushbackMask_.z);
}

nlohmann::json ColliderComponent::Serialize() {
    nlohmann::json j;
    j["isTrigger"] = isTrigger_;
    j["isStatic"] = isStatic_;
    j["layer"] = layer_;
    j["mask"] = mask_;
    j["pushbackMask"] = {pushbackMask_.x, pushbackMask_.y, pushbackMask_.z};
    return j;
}

void ColliderComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("isTrigger")) {
        isTrigger_ = j["isTrigger"];
    }
    if (j.contains("isStatic")) {
        isStatic_ = j["isStatic"];
    }
    if (j.contains("layer")) {
        layer_ = j["layer"];
    }
    if (j.contains("mask")) {
        mask_ = j["mask"];
    }
    if (j.contains("pushbackMask") && j["pushbackMask"].is_array() && j["pushbackMask"].size() == 3) {
        pushbackMask_.x = j["pushbackMask"][0];
        pushbackMask_.y = j["pushbackMask"][1];
        pushbackMask_.z = j["pushbackMask"][2];
    }
}
