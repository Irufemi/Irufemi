#include "Player/TargetableComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/ContainerUtility.h"
#include <algorithm>

std::vector<TargetableComponent*> TargetableComponent::targets_;
std::mutex TargetableComponent::targetsMutex_;

TargetableComponent::~TargetableComponent() {
    std::lock_guard<std::mutex> lock(targetsMutex_);
    Irufemi::Container::EraseSwap(targets_, this);
}

void TargetableComponent::OnEnable() {
    std::lock_guard<std::mutex> lock(targetsMutex_);
    Irufemi::Container::PushBackUnique(targets_, this);
}

void TargetableComponent::OnDisable() {
    std::lock_guard<std::mutex> lock(targetsMutex_);
    Irufemi::Container::EraseSwap(targets_, this);
}

std::vector<TargetableComponent*> TargetableComponent::GetTargets() {
    std::lock_guard<std::mutex> lock(targetsMutex_);
    return targets_;
}

void TargetableComponent::ClearAllTargets() {
    std::lock_guard<std::mutex> lock(targetsMutex_);
    targets_.clear();
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
