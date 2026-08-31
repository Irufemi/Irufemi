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
