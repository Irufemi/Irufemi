#include "Player/TargetableComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/ContainerUtility.h"
#include <algorithm>

std::vector<TargetableComponent*> TargetableComponent::s_targets;

TargetableComponent::~TargetableComponent() {
    Irufemi::Container::EraseSwap(s_targets, this);
}

void TargetableComponent::OnEnable() {
    Irufemi::Container::PushBackUnique(s_targets, this);
}

void TargetableComponent::OnDisable() {
    Irufemi::Container::EraseSwap(s_targets, this);
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
