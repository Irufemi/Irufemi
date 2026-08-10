#include "TargetableComponent.h"
#include <algorithm>

std::vector<TargetableComponent*> TargetableComponent::s_targets;

TargetableComponent::~TargetableComponent() {
    auto it = std::find(s_targets.begin(), s_targets.end(), this);
    if (it != s_targets.end()) {
        s_targets.erase(it);
    }
}

void TargetableComponent::OnEnable() {
    auto it = std::find(s_targets.begin(), s_targets.end(), this);
    if (it == s_targets.end()) {
        s_targets.push_back(this);
    }
}

void TargetableComponent::OnDisable() {
    auto it = std::find(s_targets.begin(), s_targets.end(), this);
    if (it != s_targets.end()) {
        s_targets.erase(it);
    }
}
