#include "Component.h"
#include "../GameObject.h"

class TransformComponent* Component::GetTransform() const {
    if (gameObject_) {
        return gameObject_->GetTransform();
    }
    return nullptr;
}
