#include "Framework/Component/Collider/ColliderComponent.h"
#include "Physics/CollisionManager.h"

ColliderComponent::~ColliderComponent() {
    if (collisionManager_) {
        collisionManager_->UnregisterCollider(this);
    }
}
