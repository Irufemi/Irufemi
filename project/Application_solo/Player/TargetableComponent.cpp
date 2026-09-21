#include "Player/TargetableComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/ContainerUtility.h"
#include <algorithm>

std::vector<TargetableComponent*> TargetableComponent::targets_;

TargetableComponent::~TargetableComponent() {
    Irufemi::Container::EraseSwap(targets_, this);
}

void TargetableComponent::OnEnable() {
    Irufemi::Container::PushBackUnique(targets_, this);
}

void TargetableComponent::OnDisable() {
    Irufemi::Container::EraseSwap(targets_, this);
}

bool TargetableComponent::IsTargetable() const {
    if (!gameObject_ || !gameObject_->GetIsActive() || gameObject_->IsDestroyed()) {
        return false;
    }
    if (predicate_) {
        return predicate_();
    }
    return true;
}
