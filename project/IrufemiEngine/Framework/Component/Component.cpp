#include "Framework/Component/Component.h"
#include "Framework/Component/ComponentFactory.h"
#include "Framework/GameObject/GameObject.h"

class TransformComponent* Component::GetTransform() const {
    if (gameObject_) {
        return gameObject_->GetTransform();
    }
    return nullptr;
}

std::shared_ptr<Component> Component::Clone() {
    auto clone = ComponentFactory::Create(GetComponentName());
    if (clone) {
        // Safe fallback for unoptimized components: JSON serialization route
        clone->Deserialize(this->Serialize());
    }
    return clone;
}
