#pragma once
#include "Framework/Component/Component.h"
#include <vector>

class TargetableComponent : public Component {
public:
    TargetableComponent() = default;
    ~TargetableComponent() override;

    void OnEnable() override;
    void OnDisable() override;

    std::string GetComponentName() const override {
        return "TargetableComponent";
    }

    static const std::vector<TargetableComponent*>& GetTargets() {
        return s_targets;
    }

private:
    static std::vector<TargetableComponent*> s_targets;
};
