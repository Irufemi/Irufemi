#include "ColliderComponent.h"
#include "Engine/Manager/CollisionManager.h"

ColliderComponent::~ColliderComponent() {
    if (collisionManager_) {
        collisionManager_->UnregisterCollider(this);
    }
}
