#include "Framework/Component/Component.h"
#include "Framework/GameObject/GameObject.h"

class TransformComponent* Component::GetTransform() const {
    if (gameObject_) {
        return gameObject_->GetTransform();
    }
    return nullptr;
}
